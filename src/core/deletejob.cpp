#include "deletejob.h"
#include "totalsizejob.h"
#include "fileinfo_p.h"

namespace Fm2 {

bool DeleteJob::deleteFile(const FilePath& path, GFileInfoPtr inf, bool only_empty) {
    GErrorPtr err;
#if 0
    GError* err = NULL;
    FmJobErrorAction act;

    while(!inf) {
        inf = GFileInfoPtr{
                g_file_query_info(path.gfile().get(), "standard::*",
                                  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                  cancellable().get(), &err),
                false
        };
        if(inf) {
            break;
        }
/*
        act = fm_job_emit_error(job, err, FM_JOB_ERROR_MODERATE);
*/
        g_error_free(err);
        err = NULL;
/*
        if(act == FM_JOB_ABORT) {
            return false;
        }
        if(act != FM_JOB_RETRY) {
            break;
        }
*/
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
        return deleteDir(path, inf, only_empty);
    }
    else {
        while(!isCancelled()) {
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
                g_error_free(err);
            }
        }
        /* show progress */
        // setCurrentFileProgress()
    }
#endif
    return false;
}

bool DeleteJob::deleteDir(const FilePath &path, GFileInfoPtr inf, bool only_empty) {
#if 0
    GError* err = NULL;
    bool is_dir, is_trash_root = false, ok;
    GFileInfo* _inf = NULL;
    FmJobErrorAction act;

        GFileEnumerator* enu;
        FmFolder* sub_folder;
#if 0
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

        g_error_free(err);
        err = NULL;
        enu = g_file_enumerate_children(path.gfile().get(), gfile_info_query_attribs,
                                        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                        cancellable().get(), &err);
/*
        if(!enu) {
            fm_job_emit_error(job, err, FM_JOB_ERROR_MODERATE);
            g_error_free(err);
            return false;
        }
*/
        path = fm_path_new_for_gfile(gf);
        sub_folder = fm_folder_find_by_path(path);
        fm_path_unref(path);
        while(! fm_job_is_cancelled(job)) {
            inf = g_file_enumerator_next_file(enu, fm_job_get_cancellable(job), &err);
            if(inf) {
                GFile* sub = g_file_get_child(gf, g_file_info_get_name(inf));
                ok = _fm_file_ops_job_delete_file(job, sub, inf, sub_folder, false);
                g_object_unref(sub);
                g_object_unref(inf);
                if(!ok) { /* stop the job if error happened */
                    goto _failed;
                }
            }
            else {
                if(err) {
                    fm_job_emit_error(job, err, FM_JOB_ERROR_MODERATE);
                    /* FM_JOB_RETRY is not supported here */
                    g_error_free(err);
_failed:
                    g_object_unref(enu);
                    if(sub_folder) {
                        g_object_unref(sub_folder);
                    }
                    return false;
                }
                else { /* EOF */
                    break;
                }
            }
        }
        g_object_unref(enu);
        if(sub_folder) {
            g_object_unref(sub_folder);
        }

        is_trash_root = false; /* don't go here again! */
        is_dir = false;
        continue;
    }
if(err) {
    /* if it's non-empty dir then descent into it then try again */
    /* trash root gives G_IO_ERROR_PERMISSION_DENIED */
    if(is_trash_root || /* FIXME: need to refactor this! */
            (is_dir && !only_empty &&
             err->domain == G_IO_ERROR && err->code == G_IO_ERROR_NOT_EMPTY)) {
        deleteDir(path, inf, only_empty);
    }
    else if(err->domain == G_IO_ERROR && err->code == G_IO_ERROR_PERMISSION_DENIED) {
        /* special case for trash:/// */
        /* FIXME: is there any better way to handle this? */
        auto scheme = path.uriScheme();
        if(g_strcmp0(scheme.get(), "trash") == 0) {
            g_error_free(err);
            return true;
        }
    }
#if 0
    act = fm_job_emit_error(job, err, FM_JOB_ERROR_MODERATE);
    g_error_free(err);
    err = NULL;
    if(act != FM_JOB_RETRY) {
        return false;
    }
#endif
}

#endif
    return false;
}


void DeleteJob::run() {
    /* prepare the job, count total work needed with FmDeepCountJob */
    TotalSizeJob totalSizeJob{paths_};
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
