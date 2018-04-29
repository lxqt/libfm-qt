#include "copyjob.h"
#include "totalsizejob.h"
#include "fileinfo_p.h"

namespace Fm {

CopyJob::CopyJob(FilePathList srcPaths, Mode mode):
    FileOperationJob{},
    srcPaths_{std::move(srcPaths)},
    mode_{mode},
    skipDirContent_{false} {
}

CopyJob::CopyJob(FilePathList srcPaths, FilePathList destPaths, Mode mode):
    CopyJob{std::move(srcPaths), mode} {
    destPaths_ = std::move(destPaths);
}

CopyJob::CopyJob(FilePathList srcPaths, const FilePath& destDirPath, Mode mode):
    CopyJob{std::move(srcPaths), mode} {
    setDestDirPath(destDirPath);
}

void CopyJob::setDestPaths(FilePathList destPaths) {
    destPaths_ = std::move(destPaths);
}

void CopyJob::setDestDirPath(const FilePath& destDirPath) {
    destPaths_.clear();
    destPaths_.reserve(srcPaths_.size());
    for(const auto& srcPath: srcPaths_) {
        destPaths_.emplace_back(destDirPath.child(srcPath.baseName().get()));
    }
}

void CopyJob::gfileCopyProgressCallback(goffset current_num_bytes, goffset total_num_bytes, CopyJob* _this) {
    _this->setCurrentFileProgress(total_num_bytes, current_num_bytes);
}

bool CopyJob::copyOrMoveFile(Mode mode, const FilePath& srcPath, const GFileInfoPtr& srcInfo, FilePath& destPath, bool& deleteSrc) {
    int flags = G_FILE_COPY_ALL_METADATA | G_FILE_COPY_NOFOLLOW_SYMLINKS;
    GErrorPtr err;
    bool retry;
    // choose whether we do copy or move here
    auto gfileOperationFunc = (mode == Mode::COPY ? g_file_copy : g_file_move);
    auto progresCallback = (mode == Mode::COPY ? GFileProgressCallback(gfileCopyProgressCallback) : nullptr);
    do {
        retry = false;
        err.reset();

        // reset progress of the current file (only for copy)
        if(mode == Mode::COPY) {
            auto size = g_file_info_get_size(srcInfo.get());
            setCurrentFileProgress(size, 0);
        }

        // do the file operation (copy or move)
        if(!gfileOperationFunc(srcPath.gfile().get(), destPath.gfile().get(), GFileCopyFlags(flags), cancellable().get(),
                               progresCallback, this, &err)) {
            flags &= ~G_FILE_COPY_OVERWRITE;
            /* handle existing files or file name conflict */
            if(err.domain() == G_IO_ERROR && (err.code() == G_IO_ERROR_EXISTS ||
                                             err.code() == G_IO_ERROR_INVALID_FILENAME ||
                                             err.code() == G_IO_ERROR_FILENAME_TOO_LONG)) {
                err.reset();

                // get info of the existing file
                GFileInfoPtr destInfo = GFileInfoPtr {
                    g_file_query_info(destPath.gfile().get(),
                    gfile_info_query_attribs,
                    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                    cancellable().get(), &err),
                    false
                };

                // ask the user to rename or overwrite the existing file
                if(!isCancelled() && destInfo) {
                    FilePath newDestPath;
                    FileExistsAction opt = askRename(FileInfo{srcInfo, srcPath.parent()},
                                                     FileInfo{destInfo, destPath.parent()},
                                                     newDestPath);
                    switch(opt) {
                    case FileOperationJob::RENAME:
                        // try a new file name
                        if(newDestPath.isValid()) {
                            destPath = std::move(newDestPath);
                            // FIXME: handle the error when newDestPath is invalid.
                        }
                        retry = true;
                        break;
                    case FileOperationJob::OVERWRITE:
                        // overwrite existing file
                        flags |= G_FILE_COPY_OVERWRITE;
                        retry = true;
                        break;
                    case FileOperationJob::CANCEL:
                        // cancel the whole job.
                        cancel();
                        break;
                    case FileOperationJob::SKIP:
                        // skip current file and don't copy it
                        deleteSrc = false; /* don't delete source file. */
                        return true;
                    case FileOperationJob::SKIP_ERROR: ; /* FIXME */
                        return true;
                    }
                    err.reset();
                }
            }

            // show error message
            if(err) {
                ErrorAction act = emitError( err, ErrorSeverity::MODERATE);
                err.reset();
                if(act == ErrorAction::RETRY) {
                    // the user wants retry the operation again
                    retry = true;
                    continue;
                }
                const bool is_no_space = (err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_NO_SPACE);
                /* FIXME: ask to leave partial content? */
                if(is_no_space) {
                    // run out of disk space. delete the partial content we copied.
                    g_file_delete(destPath.gfile().get(), cancellable().get(), nullptr);
                }
                deleteSrc = false;
            }
        }
        else {
            return true;
        }
    } while(retry && !isCancelled());
    return false;
}

bool CopyJob::copySpecialFile(const FilePath& srcPath, const GFileInfoPtr& srcInfo, FilePath &destPath) {
    bool ret = false;
    // only handle FIFO for local files
    if(srcPath.isNative() && destPath.isNative()) {
        auto src_path = srcPath.localPath();
        struct stat src_st;
        int r;
        r = lstat(src_path.get(), &src_st);
        if(r == 0) {
            // Handle FIFO on native file systems.
            if(S_ISFIFO(src_st.st_mode)) {
                auto dest_path = destPath.localPath();
                if(mkfifo(dest_path.get(), src_st.st_mode) == 0) {
                    ret = true;
                }
            }
            // FIXME: how about block device, char device, and socket?
        }
    }
    if(!ret) {
        GErrorPtr err;
        g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                    ("Cannot copy file '%s': not supported"),
                    g_file_info_get_display_name(srcInfo.get()));
        emitError(err, ErrorSeverity::MODERATE);
    }
    return ret;
}

bool CopyJob::copyDir(const FilePath& srcPath, GFileInfoPtr srcInfo, FilePath& destPath, bool& deleteSrc) {
    bool ret = false;
    if(makeDir(srcPath, srcInfo, destPath)) {
        GErrorPtr err;
        auto enu = GFileEnumeratorPtr{
                g_file_enumerate_children(srcPath.gfile().get(),
                                          gfile_info_query_attribs,
                                          G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                          cancellable().get(), &err),
                false};
        if(enu) {
            int n_children = 0;
            int n_copied = 0;
            ret = true;
            while(!isCancelled()) {
                err.reset();
                GFileInfoPtr inf{g_file_enumerator_next_file(enu.get(), cancellable().get(), &err), false};
                if(inf) {
                    ++n_children;
                    /* don't overwrite dir content, only calculate progress. */
                    if(Q_UNLIKELY(skipDirContent_)) {
                        /* FIXME: this is incorrect as we don't do the calculation recursively. */
                        addFinishedAmount(g_file_info_get_size(inf.get()), 1);
                    }
                    else {
                        const char* name = g_file_info_get_name(inf.get());
                        FilePath childPath = srcPath.child(name);
                        bool child_ret = copyPath(childPath, inf, destPath, name, deleteSrc);
                        if(child_ret) {
                            ++n_copied;
                        }
                        else {
                            ret = false;
                        }
                    }
                }
                else {
                    if(err) {
                        emitError(err, ErrorSeverity::MODERATE);
                        err.reset();
                        /* ErrorAction::RETRY is not supported here */
                        ret = false;
                    }
                    else { /* EOF is reached */
                        /* all files are successfully copied. */
                        if(isCancelled()) {
                            ret = false;
                        }
                        else {
                            /* some files are not copied */
                            if(n_children != n_copied) {
                                /* if the copy actions are skipped deliberately, it's ok */
                                if(!skipDirContent_) {
                                    ret = false;
                                }
                            }
                            /* else job->skip_dir_content is true */
                        }
                        break;
                    }
                }
            }
            g_file_enumerator_close(enu.get(), nullptr, &err);
        }
        else {
            if(err) {
                emitError(err, ErrorSeverity::MODERATE);
            }
        }
    }
    return ret;
}

bool CopyJob::makeDir(const FilePath& srcPath, GFileInfoPtr srcInfo, FilePath& destPath) {
    if(isCancelled()) {
        return false;
    }

    bool mkdir_done = false;
    do {
        GErrorPtr err;
        mkdir_done = g_file_make_directory(destPath.gfile().get(), cancellable().get(), &err);
        if(!mkdir_done) {
            if(err->domain == G_IO_ERROR && (err->code == G_IO_ERROR_EXISTS ||
                                             err->code == G_IO_ERROR_INVALID_FILENAME ||
                                             err->code == G_IO_ERROR_FILENAME_TOO_LONG)) {
                GFileInfoPtr destInfo = GFileInfoPtr {
                    g_file_query_info(destPath.gfile().get(),
                    gfile_info_query_attribs,
                    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                    cancellable().get(), nullptr),
                    false
                };
                if(!destInfo) {
                    // FIXME: error handling
                    break;
                }

                FilePath newDestPath;
                FileExistsAction opt = askRename(FileInfo{srcInfo, srcPath.parent()}, FileInfo{destInfo, destPath.parent()}, newDestPath);
                switch(opt) {
                case FileOperationJob::RENAME:
                    destPath = std::move(newDestPath);
                    break;
                case FileOperationJob::SKIP:
                    /* when a dir is skipped, we need to know its total size to calculate correct progress */
                    skipDirContent_ = true;
                    mkdir_done = true; /* pretend that dir creation succeeded */
                    break;
                case FileOperationJob::OVERWRITE:
                    mkdir_done = true; /* pretend that dir creation succeeded */
                    break;
                case FileOperationJob::CANCEL:
                    cancel();
                    return false;
                case FileOperationJob::SKIP_ERROR: ; /* FIXME */
                }
            }
            else {
                ErrorAction act = emitError(err, ErrorSeverity::MODERATE);
                if(act != ErrorAction::RETRY) {
                    break;
                }
            }
        }
    } while(!mkdir_done && !isCancelled());

    bool chmod_done = false;
    if(mkdir_done && !isCancelled()) {
        mode_t mode = g_file_info_get_attribute_uint32(srcInfo.get(), G_FILE_ATTRIBUTE_UNIX_MODE);
        if(mode) {
            mode |= (S_IRUSR | S_IWUSR); /* ensure we have rw permission to this file. */
            do {
                GErrorPtr err;
                // chmod the newly created dir properly
                // if(!fm_job_is_cancelled(fmjob) && !job->skip_dir_content)
                chmod_done = g_file_set_attribute_uint32(destPath.gfile().get(),
                                                         G_FILE_ATTRIBUTE_UNIX_MODE,
                                                         mode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                         cancellable().get(), &err);
                if(!chmod_done) {
                    ErrorAction act = emitError(err, ErrorSeverity::MODERATE);
                    if(act != ErrorAction::RETRY) {
                        break;
                    }
                    /* FIXME: some filesystems may not support this. */
                }
            } while(!chmod_done && !isCancelled());
        }
    }

    addFinishedAmount(g_file_info_get_size(srcInfo.get()), 1);
    return mkdir_done && chmod_done;
}

bool CopyJob::processPath(const FilePath& srcPath, const FilePath& destDirPath, const char* destFileName) {
    setCurrentFile(srcPath);

    GErrorPtr err;
    GFileInfoPtr srcInfo = GFileInfoPtr {
        g_file_query_info(srcPath.gfile().get(),
        gfile_info_query_attribs,
        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
        cancellable().get(), &err),
        false
    };
    if(!srcInfo || isCancelled()) {
        // FIXME: report error
        return false;
    }

    bool ret;
    if(mode_ == Mode::MOVE) {
        ret = movePath(srcPath, srcInfo, destDirPath, destFileName);
    }
    else {
        bool deleteSrc = false;
        ret = copyPath(srcPath, srcInfo, destDirPath, destFileName, deleteSrc);
    }
    return ret;
}

bool CopyJob::movePath(const FilePath &srcPath, const GFileInfoPtr &srcInfo, const FilePath &destDirPath, const char *destFileName) {
    GErrorPtr err;
    GFileInfoPtr destDirInfo = GFileInfoPtr {
        g_file_query_info(destDirPath.gfile().get(),
        "id::filesystem",
        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
        cancellable().get(), &err),
        false
    };

    if(!destDirInfo || isCancelled()) {
        // FIXME: report errors
        return false;
    }

    // If src and dest are on the same filesystem, do move.
    // Otherwise, do copy & delete src files.
    auto src_fs = g_file_info_get_attribute_string(srcInfo.get(), "id::filesystem");
    auto dest_fs = g_file_info_get_attribute_string(destDirInfo.get(), "id::filesystem");
    bool ret;
    bool deleteSrc;
    if(g_strcmp0(src_fs, dest_fs) == 0) {
        // src and dest are on the same filesystem
        auto destPath = destDirPath.child(destFileName);
        deleteSrc = false;
        ret = copyOrMoveFile(Mode::MOVE, srcPath, srcInfo, destPath, deleteSrc);
    }
    else {
        deleteSrc = true;
        // cross device/filesystem move: copy & delete
        ret = copyPath(srcPath, srcInfo, destDirPath, destFileName, deleteSrc);
    }
    return ret;
}

bool CopyJob::copyPath(const FilePath& srcPath, const GFileInfoPtr& srcInfo, const FilePath& destDirPath, const char* destFileName, bool& deleteSrc) {
    // FIXME: set this to true for cross-device move
    auto size = g_file_info_get_size(srcInfo.get());
    setCurrentFileProgress(size, 0);

    auto destPath = destDirPath.child(destFileName);
    bool success = false;
    switch(g_file_info_get_file_type(srcInfo.get())) {
    case G_FILE_TYPE_DIRECTORY:
        success = copyDir(srcPath, srcInfo, destPath, deleteSrc);
        break;
    case G_FILE_TYPE_SPECIAL:
        success = copySpecialFile(srcPath, srcInfo, destPath);
        break;
    default:
        success = copyOrMoveFile(Mode::COPY, srcPath, srcInfo, destPath, deleteSrc);
        break;
    }

    if(success) {
        addFinishedAmount(size, 1);

        if(deleteSrc) {
            // delete the source file for cross-filesystem move
            // TODO: delete the src file
        }
#if 0

        if(ret && dest_folder) {
            fm_dest = fm_path_new_for_gfile(dest);
            if(!_fm_folder_event_file_added(dest_folder, fm_dest)) {
                fm_path_unref(fm_dest);
            }
        }
#endif
    }

    return success;
}

#if 0

bool _fm_file_ops_job_copy_run(FmFileOpsJob* job) {
    bool ret = true;
    GFile* dest_dir;
    GList* l;
    FmJob* fmjob = FM_JOB(job);
    /* prepare the job, count total work needed with FmDeepCountJob */
    FmDeepCountJob* dc = fm_deep_count_job_new(job->srcs, FM_DC_JOB_DEFAULT);
    FmFolder* df;

    /* let the deep count job share the same cancellable object. */
    fm_job_set_cancellable(FM_JOB(dc), fm_job_get_cancellable(fmjob));
    fm_job_run_sync(FM_JOB(dc));
    job->total = dc->total_size;
    if(fm_job_is_cancelled(fmjob)) {
        g_object_unref(dc);
        return false;
    }
    g_object_unref(dc);
    g_debug("total size to copy: %llu", (long long unsigned int)job->total);

    dest_dir = fm_path_to_gfile(job->dest);
    /* suspend updates for destination */
    df = fm_folder_find_by_path(job->dest);
    if(df) {
        fm_folder_block_updates(df);
    }

    fm_file_ops_job_emit_prepared(job);

    for(l = fm_path_list_peek_head_link(job->srcs); !fm_job_is_cancelled(fmjob) && l; l = l->next) {
        FmPath* path = FM_PATH(l->data);
        GFile* src = fm_path_to_gfile(path);
        GFile* dest;
        char* tmp_basename;

        if(g_file_is_native(src) && g_file_is_native(dest_dir))
            /* both are native */
        {
            tmp_basename = nullptr;
        }
        else if(g_file_is_native(src)) /* copy from native to virtual */
            tmp_basename = g_filename_to_utf8(fm_path_get_basename(path),
                                              -1, nullptr, nullptr, nullptr);
        /* gvfs escapes it itself */
        else { /* copy from virtual to native/virtual */
            /* if we drop URI query onto native filesystem, omit query part */
            const char* basename = fm_path_get_basename(path);
            char* sub_name;

            sub_name = strchr(basename, '?');
            if(sub_name) {
                sub_name = g_strndup(basename, sub_name - basename);
                basename = strrchr(sub_name, G_DIR_SEPARATOR);
                if(basename) {
                    basename++;
                }
                else {
                    basename = sub_name;
                }
            }
            tmp_basename = fm_uri_subpath_to_native_subpath(basename, nullptr);
            g_free(sub_name);
        }
        dest = g_file_get_child(dest_dir,
                                tmp_basename ? tmp_basename : fm_path_get_basename(path));
        g_free(tmp_basename);
        if(!_fm_file_ops_job_copy_file(job, src, nullptr, dest, nullptr, df)) {
            ret = false;
        }
        g_object_unref(src);
        g_object_unref(dest);
    }

    /* g_debug("finished: %llu, total: %llu", job->finished, job->total); */
    fm_file_ops_job_emit_percent(job);

    /* restore updates for destination */
    if(df) {
        fm_folder_unblock_updates(df);
        g_object_unref(df);
    }
    g_object_unref(dest_dir);
    return ret;
}
#endif

void CopyJob::exec() {
    // calculate the total size of files to copy
    auto totalSizeFlags = (mode_ == Mode::COPY ? TotalSizeJob::DEFAULT : TotalSizeJob::PREPARE_MOVE);
    TotalSizeJob totalSizeJob{srcPaths_, totalSizeFlags};
    connect(&totalSizeJob, &TotalSizeJob::error, this, &CopyJob::error);
    connect(this, &CopyJob::cancelled, &totalSizeJob, &TotalSizeJob::cancel);
    totalSizeJob.run();
    if(isCancelled()) {
        return;
    }

    // ready to start
    setTotalAmount(totalSizeJob.totalSize(), totalSizeJob.fileCount());
    Q_EMIT preparedToRun();

    if(srcPaths_.size() != destPaths_.size()) {
        qWarning("error: srcPaths.size() != destPaths.size() when copying files");
        return;
    }

    // copy the files
    for(size_t i = 0; i < srcPaths_.size(); ++i) {
        if(isCancelled()) {
            break;
        }
        const auto& srcPath = srcPaths_[i];
        const auto& destPath = destPaths_[i];
        auto destDirPath = destPath.parent();
        processPath(srcPath, destDirPath, destPath.baseName().get());
    }
}


} // namespace Fm
