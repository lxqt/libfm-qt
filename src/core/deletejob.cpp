#include "deletejob.h"
#include "totalsizejob.h"
#include "fileinfo_p.h"

namespace Fm2 {

bool DeleteJob::deleteFile(const FilePath& path, GFileInfoPtr inf, bool only_empty) {
    ErrorAction act = ErrorAction::CONTINUE;
    while(!inf) {
        GErrorPtr err;
        inf = GFileInfoPtr{
                g_file_query_info(path.gfile().get(), "standard::*",
                                  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                  cancellable().get(), &err),
                false
        };
        if(inf) {
            break;
        }
        act = emitError(err, ErrorSeverity::MODERATE);
        if(act == ErrorAction::ABORT) {
            return false;
        }
        if(act != ErrorAction::RETRY) {
            break;
        }
    }
    if(!inf) {
#if 0
        /* use basename of GFile as display name. */
        char* basename = g_file_get_basename(gf);
        char* disp = basename ? g_filename_display_name(basename) : NULL;
        g_free(basename);
        /* FIXME: translate it */
        fm_file_ops_job_emit_cur_file(fjob, disp ? disp : "(invalid file)");
        g_free(disp);
        ++fjob->finished;
#endif
        return false;
    }

    /* currently processed file. */
    setCurrentFile(path);

    if(g_file_info_get_file_type(inf.get()) == G_FILE_TYPE_DIRECTORY) {
        // delete the content of the dir prior to deleting itself
        if(!deleteDirContent(path, inf, only_empty))
            return false;
    }

    while(!isCancelled()) {
        GErrorPtr err;
        // try to delete the path directly
        if(g_file_delete(path.gfile().get(), cancellable().get(), &err)) {
/*
            if(folder) {
                path = fm_path_new_for_gfile(gf);
                _fm_folder_event_file_deleted(folder, path);
                fm_path_unref(path);
            }
*/
            return true;
        }
        if(err) {
            // FIXME: error handling
#if 0
            /* if it's non-empty dir then descent into it then try again */
            /* trash root gives G_IO_ERROR_PERMISSION_DENIED */
            if(is_trash_root || /* FIXME: need to refactor this! */
                    (is_dir && !only_empty &&
                     err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_NOT_EMPTY)) {
                deleteDirContent(path, inf, only_empty);
            }
            else if(err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_PERMISSION_DENIED) {
                /* special case for trash:/// */
                /* FIXME: is there any better way to handle this? */
                auto scheme = path.uriScheme();
                if(g_strcmp0(scheme.get(), "trash") == 0) {
                    return true;
                }
            }
#endif
            act = emitError( err, ErrorSeverity::MODERATE);
            if(act != ErrorAction::RETRY) {
                return false;
            }
        }
    }
    /* show progress */
    // setCurrentFileProgress()
    return false;
}

bool DeleteJob::deleteDirContent(const FilePath &path, GFileInfoPtr inf, bool only_empty) {
    GErrorPtr err;
    bool is_dir, is_trash_root = false, ok;
    ErrorAction act;

#if 0
    FmFolder* sub_folder;
        /* special handling for trash:/// */
        if(!g_file_is_native(gf)) {
            char* scheme = g_file_get_uri_scheme(gf);
            if(g_strcmp0(scheme, "trash") == 0) {
                /* little trick: basename of trash root is /. */
                char* basename = g_file_get_basename(gf);
                if(basename && basename[0] == G_DIR_SEPARATOR) {
                    is_trash_root = true;
                }
                g_free(basename);
            }
            g_free(scheme);
        }
#endif
    GFileEnumeratorPtr enu{
        g_file_enumerate_children(path.gfile().get(), gfile_info_query_attribs,
                                G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                cancellable().get(), &err),
                false
    };
    if(!enu) {
        emitError(err, ErrorSeverity::MODERATE);
        return false;
    }
    // FIXME: sub_folder = fm_folder_find_by_path(path);
    while(!isCancelled()) {
        inf = GFileInfoPtr{
            g_file_enumerator_next_file(enu.get(), cancellable().get(), &err),
            false
        };
        if(inf) {
            auto subPath = path.child(g_file_info_get_name(inf.get()));
            if(!deleteFile(subPath, inf, false))
                goto _failed;
        }
        else {
            if(err) {
                emitError( err, ErrorSeverity::MODERATE);
                /* ErrorAction::RETRY is not supported here */
_failed:
                g_file_enumerator_close(enu.get(), nullptr, nullptr);
/*
                if(sub_folder) {
                    g_object_unref(sub_folder);
                }
*/
                return false;
            }
            else { /* EOF */
                g_file_enumerator_close(enu.get(), nullptr, nullptr);
                break;
            }
        }
        is_trash_root = false; /* don't go here again! */
        is_dir = false;
    }
    return false;
}


void DeleteJob::run() {
    /* prepare the job, count total work needed with FmDeepCountJob */
    TotalSizeJob totalSizeJob{paths_, TotalSizeJob::Flags::PREPARE_DELETE};
    connect(&totalSizeJob, &TotalSizeJob::error, this, &DeleteJob::error);
    connect(this, &DeleteJob::cancelled, &totalSizeJob, &TotalSizeJob::cancel);
    totalSizeJob.run();

    if(isCancelled()) {
        return;
    }

    setTotalAmount(totalSizeJob.totalSize(), totalSizeJob.fileCount());
    Q_EMIT preparedToRun();

    for(auto& path: paths_) {
        if(isCancelled())
            break;
/*
        if(fm_path_get_parent(path) != parent && fm_path_get_parent(path) != NULL) {
            FmFolder* pf = fm_folder_find_by_path(fm_path_get_parent(path));
            if(pf != parent_folder) {
                if(parent_folder) {
                    fm_folder_unblock_updates(parent_folder);
                    g_object_unref(parent_folder);
                }
                if(pf) {
                    fm_folder_block_updates(pf);
                }
                parent_folder = pf;
            }
            else if(pf) {
                g_object_unref(pf);
            }
        }
*/
        deleteFile(path, GFileInfoPtr{nullptr}, false);
    }
/*
    if(parent_folder) {
        fm_folder_unblock_updates(parent_folder);
        g_object_unref(parent_folder);
    }
*/
}

} // namespace Fm2
