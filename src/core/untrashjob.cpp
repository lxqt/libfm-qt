#include "untrashjob.h"

namespace Fm2 {

UntrashJob::UntrashJob() {

}

static const char trash_query[] =
    G_FILE_ATTRIBUTE_STANDARD_TYPE","
    G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME","
    G_FILE_ATTRIBUTE_STANDARD_NAME","
    G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL","
    G_FILE_ATTRIBUTE_STANDARD_SIZE","
    G_FILE_ATTRIBUTE_UNIX_BLOCKS","
    G_FILE_ATTRIBUTE_UNIX_BLOCK_SIZE","
    G_FILE_ATTRIBUTE_ID_FILESYSTEM","
    "trash::orig-path";

static gboolean ensure_parent_dir(FmJob* job, GFile* orig_path) {
    GFile* parent = g_file_get_parent(orig_path);
    gboolean ret = g_file_query_exists(parent, fm_job_get_cancellable(job));
    if(!ret) {
        GError* err = NULL;
_retry_mkdir:
        if(!g_file_make_directory_with_parents(parent, fm_job_get_cancellable(job), &err)) {
            if(!fm_job_is_cancelled(job)) {
                FmJobErrorAction act = fm_job_emit_error(job, err, FM_JOB_ERROR_MODERATE);
                g_error_free(err);
                err = NULL;
                if(act == FM_JOB_RETRY) {
                    goto _retry_mkdir;
                }
            }
        }
        else {
            ret = TRUE;
        }
    }
    g_object_unref(parent);
    return ret;
}


void UntrashJob::run() {
#if 0
    gboolean ret = TRUE;
    GList* l;
    GError* err = NULL;
    FmJob* fmjob = FM_JOB(job);
    job->total = fm_path_list_get_length(job->srcs);
    fm_file_ops_job_emit_prepared(job);

    l = fm_path_list_peek_head_link(job->srcs);
    for(; !fm_job_is_cancelled(fmjob) && l; l = l->next) {
        GFile* gf;
        GFileInfo* inf;
        FmPath* path = FM_PATH(l->data);
        if(!fm_path_is_trash(path)) {
            continue;
        }
        gf = fm_path_to_gfile(path);
_retry_get_orig_path:
        inf = g_file_query_info(gf, trash_query, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, fm_job_get_cancellable(fmjob), &err);
        if(inf) {
            const char* orig_path_str = g_file_info_get_attribute_byte_string(inf, "trash::orig-path");
            fm_file_ops_job_emit_cur_file(job, g_file_info_get_display_name(inf));

            if(orig_path_str) {
                /* FIXME: what if orig_path_str is a relative path?
                 * This is actually allowed by the horrible trash spec. */
                GFile* orig_path = fm_file_new_for_commandline_arg(orig_path_str);
                FmFolder* src_folder = fm_folder_find_by_path(fm_path_get_parent(path));
                FmPath* orig_fm_path = fm_path_new_for_gfile(orig_path);
                FmFolder* dst_folder = fm_folder_find_by_path(fm_path_get_parent(orig_fm_path));
                fm_path_unref(orig_fm_path);
                /* ensure the existence of parent folder. */
                if(ensure_parent_dir(fmjob, orig_path)) {
                    ret = _fm_file_ops_job_move_file(job, gf, inf, orig_path, path, src_folder, dst_folder);
                }
                if(src_folder) {
                    g_object_unref(src_folder);
                }
                if(dst_folder) {
                    g_object_unref(dst_folder);
                }
                g_object_unref(orig_path);
            }
            else {
                FmJobErrorAction act;

                g_set_error(&err, G_IO_ERROR, G_IO_ERROR_FAILED,
                            _("Cannot untrash file '%s': original path not known"),
                            g_file_info_get_display_name(inf));
                act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MODERATE);
                g_clear_error(&err);
                if(act == FM_JOB_ABORT) {
                    g_object_unref(inf);
                    g_object_unref(gf);
                    return FALSE;
                }
            }
            g_object_unref(inf);
        }
        else {
            char* basename = g_file_get_basename(gf);
            char* disp = basename ? g_filename_display_name(basename) : NULL;
            g_free(basename);
            /* FIXME: translate it */
            fm_file_ops_job_emit_cur_file(job, disp ? disp : "(invalid file)");
            g_free(disp);

            if(err) {
                FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MODERATE);
                g_error_free(err);
                err = NULL;
                if(act == FM_JOB_RETRY) {
                    goto _retry_get_orig_path;
                }
                else if(act == FM_JOB_ABORT) {
                    g_object_unref(gf);
                    return FALSE;
                }
            }
        }
        g_object_unref(gf);
        ++job->finished;
        fm_file_ops_job_emit_percent(job);
    }
#endif
}

} // namespace Fm2
