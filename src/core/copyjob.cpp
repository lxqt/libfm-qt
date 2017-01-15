#include "copyjob.h"
#include "totalsizejob.h"
#include "fileinfo_p.h"

namespace Fm2 {

CopyJob::CopyJob(const FilePathList& paths, const FilePath& destDirPath, Mode mode):
    FileOperationJob{},
    srcPaths_{paths},
    destDirPath_{destDirPath},
    mode_{mode},
    skip_dir_content{false} {
}

CopyJob::CopyJob(const FilePathList &&paths, const FilePath &&destDirPath, Mode mode):
    FileOperationJob{},
    srcPaths_{paths},
    destDirPath_{destDirPath},
    mode_{mode},
    skip_dir_content{false} {
}

void CopyJob::gfileProgressCallback(goffset current_num_bytes, goffset total_num_bytes, CopyJob* _this) {
    _this->setCurrentFileProgress(total_num_bytes, current_num_bytes);
}

bool CopyJob::copyRegularFile(const FilePath& srcPath, GFileInfoPtr srcFile, const FilePath& destPath) {
    int flags = G_FILE_COPY_ALL_METADATA | G_FILE_COPY_NOFOLLOW_SYMLINKS;
    GErrorPtr err;
_retry_copy:
    if(!g_file_copy(srcPath.gfile().get(), destPath.gfile().get(), GFileCopyFlags(flags), cancellable().get(),
                    GFileProgressCallback(gfileProgressCallback), this, &err)) {
        flags &= ~G_FILE_COPY_OVERWRITE;
        /* handle existing files or file name conflict */
        if(err.domain() == G_IO_ERROR && (err.code() == G_IO_ERROR_EXISTS ||
                                         err.code() == G_IO_ERROR_INVALID_FILENAME ||
                                         err.code() == G_IO_ERROR_FILENAME_TOO_LONG)) {
#if 0
            GFile* dest_cp = new_dest;
            bool dest_exists = (err->code == G_IO_ERROR_EXISTS);
            FmFileOpOption opt = 0;
            g_error_free(err);
            err = NULL;

            new_dest = NULL;
            opt = _fm_file_ops_job_ask_new_name(job, src, dest, &new_dest, dest_exists);
            if(!new_dest) { /* restoring status quo */
                new_dest = dest_cp;
            }
            else if(dest_cp) { /* we got new new_dest, forget old one */
                g_object_unref(dest_cp);
            }
            switch(opt) {
            case FM_FILE_OP_RENAME:
                dest = new_dest;
                goto _retry_copy;
                break;
            case FM_FILE_OP_OVERWRITE:
                flags |= G_FILE_COPY_OVERWRITE;
                goto _retry_copy;
                break;
            case FM_FILE_OP_CANCEL:
                fm_job_cancel(fmjob);
                break;
            case FM_FILE_OP_SKIP:
                ret = true;
                delete_src = false; /* don't delete source file. */
                break;
            case FM_FILE_OP_SKIP_ERROR: ; /* FIXME */
            }
#endif
        }
        else {
            bool is_no_space = (err.domain() == G_IO_ERROR &&
                                err.code() == G_IO_ERROR_NO_SPACE);
            ErrorAction act = emitError( err, ErrorSeverity::MODERATE);
            err.reset();
            if(act == ErrorAction::RETRY) {
                // FIXME: job->current_file_finished = 0;
                goto _retry_copy;
            }
# if 0
            /* FIXME: ask to leave partial content? */
            if(is_no_space) {
                g_file_delete(dest, fm_job_get_cancellable(fmjob), NULL);
            }
            ret = false;
            delete_src = false;
#endif
        }
        err.reset();
    }
    else {
        return true;
    }
    return false;
}

bool CopyJob::copySpecialFile(const FilePath& srcPath, GFileInfoPtr srcFile, const FilePath& destPath) {
    bool ret = false;
    GError* err = nullptr;
    /* only handle FIFO for local files */
    if(srcPath.isNative() && destPath.isNative()) {
        auto src_path = srcPath.localPath();
        struct stat src_st;
        int r;
        r = lstat(src_path.get(), &src_st);
        if(r == 0) {
            /* Handle FIFO on native file systems. */
            if(S_ISFIFO(src_st.st_mode)) {
                auto dest_path = destPath.localPath();
                if(mkfifo(dest_path.get(), src_st.st_mode) == 0) {
                    ret = true;
                }
            }
            /* FIXME: how about block device, char device, and socket? */
        }
    }
    if(!ret) {
        g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                    ("Cannot copy file '%s': not supported"),
                    g_file_info_get_display_name(srcFile.get()));
        // emitError( err, ErrorSeverity::MODERATE);
        g_clear_error(&err);
    }
    return ret;
}

bool CopyJob::copyDir(const FilePath& srcPath, GFileInfoPtr srcFile, const FilePath& destPath) {
    bool ret = false;
    if(makeDir(srcPath, srcFile, destPath)) {
        GError* err = nullptr;
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
                auto inf = GFileInfoPtr{g_file_enumerator_next_file(enu.get(), cancellable().get(), &err), false};
                if(inf) {
                    ++n_children;
                    /* don't overwrite dir content, only calculate progress. */
                    if(Q_UNLIKELY(skip_dir_content)) {
                        /* FIXME: this is incorrect as we don't do the calculation recursively. */
                        addFinishedAmount(g_file_info_get_size(inf.get()), 1);
                    }
                    else {
                        const char* name = g_file_info_get_name(inf.get());
                        FilePath childPath = srcPath.child(name);
                        bool child_ret = copyPath(childPath, inf, destPath, name);
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
                        // FIXME: emitError( err, ErrorSeverity::MODERATE);
                        g_error_free(err);
                        err = NULL;
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
                                if(!skip_dir_content) {
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
    }
    return false;
}

bool CopyJob::makeDir(const FilePath& srcPath, GFileInfoPtr srcFile, const FilePath& dirPath) {
    GError* err = nullptr;
    if(isCancelled())
        return false;

    FilePath destPath = dirPath;
    bool mkdir_done = false;
    do {
        mkdir_done = g_file_make_directory(destPath.gfile().get(), cancellable().get(), &err);
        if(err->domain == G_IO_ERROR && (err->code == G_IO_ERROR_EXISTS ||
                                         err->code == G_IO_ERROR_INVALID_FILENAME ||
                                         err->code == G_IO_ERROR_FILENAME_TOO_LONG)) {
            bool dest_exists = (err->code == G_IO_ERROR_EXISTS);
            GFileInfoPtr destFile;
            // FIXME: query its info
            FilePath newDestPath;
            FileExistsAction opt = askRename(FileInfo{srcFile, srcPath.parent()}, FileInfo{destFile, dirPath.parent()}, newDestPath);
            g_error_free(err);
            err = NULL;

            switch(opt) {
            case FileOperationJob::RENAME:
                destPath = newDestPath;
                break;
            case FileOperationJob::SKIP:
                /* when a dir is skipped, we need to know its total size to calculate correct progress */
                // job->finished += size;
                // fm_file_ops_job_emit_percent(job);
                // job->skip_dir_content = skip_dir_content = true;
                mkdir_done = true; /* pretend that dir creation succeeded */
                break;
            case FileOperationJob::OVERWRITE:
                mkdir_done = true; /* pretend that dir creation succeeded */
                break;
            case FileOperationJob::CANCEL:
                cancel();
                break;
            case FileOperationJob::SKIP_ERROR: ; /* FIXME */
            }
        }
        else {
#if 0
            ErrorAction act = emitError( err, ErrorSeverity::MODERATE);
            g_error_free(err);
            err = NULL;
            if(act == ErrorAction::RETRY) {
                goto _retry_mkdir;
            }
#endif
            break;
        }
        // job->finished += size;
    } while(!mkdir_done && !isCancelled());

    if(mkdir_done && !isCancelled()) {
        bool chmod_done = false;
        mode_t mode = g_file_info_get_attribute_uint32(srcFile.get(), G_FILE_ATTRIBUTE_UNIX_MODE);
        if(mode) {
            mode |= (S_IRUSR | S_IWUSR); /* ensure we have rw permission to this file. */
            do {
                /* chmod the newly created dir properly */
                // if(!fm_job_is_cancelled(fmjob) && !job->skip_dir_content)
                chmod_done = g_file_set_attribute_uint32(destPath.gfile().get(),
                                                         G_FILE_ATTRIBUTE_UNIX_MODE,
                                                         mode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                         cancellable().get(), &err);
                if(!chmod_done) {
/*
                    ErrorAction act = emitError( err, ErrorSeverity::MODERATE);
                    g_error_free(err);
                    err = NULL;
                    if(act == ErrorAction::RETRY) {
                        goto _retry_chmod_for_dir;
                    }
*/
                    /* FIXME: some filesystems may not support this. */
                }
            } while(!chmod_done && !isCancelled());
            // finished += size;
            // fm_file_ops_job_emit_percent(job);
        }
    }
    return false;
}

bool CopyJob::copyPath(const FilePath& srcPath, const FilePath& destDirPath, const char* destFileName) {
    GErrorPtr err;
    GFileInfoPtr srcInfo = GFileInfoPtr {
        g_file_query_info(srcPath.gfile().get(),
        gfile_info_query_attribs,
        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
        cancellable().get(), &err),
        false
    };
    if(!srcInfo || isCancelled()) {
        return false;
    }
    return copyPath(srcPath, srcInfo, destDirPath, destFileName);
}

bool CopyJob::copyPath(const FilePath& srcPath, const GFileInfoPtr& srcInfo, const FilePath& destDirPath, const char* destFileName) {
    setCurrentFile(srcPath);
    GErrorPtr err;
    GFileInfoPtr destDirInfo = GFileInfoPtr {
        g_file_query_info(destDirPath.gfile().get(),
        "id::filesystem",
        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
        cancellable().get(), &err),
        false
    };

    if(!destDirInfo || isCancelled()) {
        return false;
    }

    auto size = g_file_info_get_size(srcInfo.get());
    setCurrentFileProgress(size, 0);

    auto destPath = destDirPath.child(destFileName);
    bool success = false;
    switch(g_file_info_get_file_type(srcInfo.get())) {
    case G_FILE_TYPE_DIRECTORY:
        success = copyDir(srcPath, srcInfo, destPath);
        break;
    case G_FILE_TYPE_SPECIAL:
        success = copySpecialFile(srcPath, srcInfo, destPath);
        break;
    default:
        success = copyRegularFile(srcPath, srcInfo, destPath);
        break;
    }

    if(success) {
        addFinishedAmount(size, 1);
#if 0

        if(ret && dest_folder) {
            fm_dest = fm_path_new_for_gfile(dest);
            if(!_fm_folder_event_file_added(dest_folder, fm_dest)) {
                fm_path_unref(fm_dest);
            }
        }
#endif
    }

    return false;
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
            tmp_basename = NULL;
        }
        else if(g_file_is_native(src)) /* copy from native to virtual */
            tmp_basename = g_filename_to_utf8(fm_path_get_basename(path),
                                              -1, NULL, NULL, NULL);
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
            tmp_basename = fm_uri_subpath_to_native_subpath(basename, NULL);
            g_free(sub_name);
        }
        dest = g_file_get_child(dest_dir,
                                tmp_basename ? tmp_basename : fm_path_get_basename(path));
        g_free(tmp_basename);
        if(!_fm_file_ops_job_copy_file(job, src, NULL, dest, NULL, df)) {
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

void CopyJob::run() {
    TotalSizeJob totalSizeJob{srcPaths_};
    connect(&totalSizeJob, &TotalSizeJob::error, this, &CopyJob::error);
    connect(this, &CopyJob::cancelled, &totalSizeJob, &TotalSizeJob::cancel);
    totalSizeJob.run();
    if(isCancelled()) {
        return;
    }

    setTotalAmount(totalSizeJob.totalSize(), totalSizeJob.fileCount());
    Q_EMIT preparedToRun();

    for(auto& srcPath : srcPaths_) {
        if(isCancelled()) {
            break;
        }
        copyPath(srcPath, destDirPath_, srcPath.baseName().get());
    }
}


} // namespace Fm2
