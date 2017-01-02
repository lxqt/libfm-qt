#include "trashjob.h"

namespace Fm2 {

TrashJob::TrashJob(const FilePathList &paths): paths_{paths} {
}

TrashJob::TrashJob(const FilePathList &&paths): paths_{paths} {
}

void TrashJob::run() {
#if 0
    GList* l;
    GError* err = NULL;
    FmPath* path, *parent = NULL;
    FmFolder* parent_folder = NULL;

    setTotalAmount(paths_.size(), paths_.size());
    Q_EMIT preparedToRun();

    /* FIXME: we shouldn't trash a file already in trash:/// */
    for(auto& path: paths_) {
        GFile* gf = path.gfile().get();
        GFileInfo* inf;
#if 0
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
        parent = fm_path_get_parent(path);
#endif
_retry_trash:
        inf = g_file_query_info(gf, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, 0,
                                fm_job_get_cancellable(fmjob), &err);
        if(inf) {
            /* currently processed file. */
            // fm_file_ops_job_emit_cur_file(job, g_file_info_get_display_name(inf));
        }
        else {
            char* basename = g_file_get_basename(gf);
            char* disp = basename ? g_filename_display_name(basename) : NULL;
            g_free(basename);
            ret = FALSE;
            /* FIXME: translate it */
            fm_file_ops_job_emit_cur_file(job, disp ? disp : "(invalid file)");
            g_free(disp);
            goto _on_error;
        }
        ret = FALSE;
        if(fm_config->no_usb_trash) {
            GMount* mnt = g_file_find_enclosing_mount(gf, NULL, &err);

            if(mnt) {
                ret = g_mount_can_unmount(mnt); /* TRUE if it's removable media */
                g_object_unref(mnt);
                if(ret) {
                    fm_path_list_push_tail(unsupported, FM_PATH(l->data));
                }
            }
            else {
                g_error_free(err);
                err = NULL;
            }
        }

        if(!ret) {
            ret = g_file_trash(gf, cancellable().get(), &err);
            if(ret && parent_folder) {
                _fm_folder_event_file_deleted(parent_folder, path);
            }
            /* FIXME: signal trash:/// that file added there */
        }
        if(!ret) {
_on_error:
            /* if trashing is not supported by the file system */
            if(err->domain == G_IO_ERROR && err->code == G_IO_ERROR_NOT_SUPPORTED) {
                unsupportedFiles_.push_back(path);
            }
            else {
                FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MODERATE);
                g_error_free(err);
                err = NULL;
                if(act == FM_JOB_RETRY) {
                    goto _retry_trash;
                }
                else if(act == FM_JOB_ABORT) {
                    return;
                }
            }
            g_error_free(err);
            err = NULL;
        }
        addFinishedAmount(1, 1);
    }
/*
    if(parent_folder) {
        fm_folder_unblock_updates(parent_folder);
        g_object_unref(parent_folder);
    }
*/

#endif
}



} // namespace Fm2
