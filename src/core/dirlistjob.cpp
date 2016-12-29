#include "dirlistjob.h"
#include <gio/gio.h>
#include "fileinfo_p.h"
#include <QDebug>

namespace Fm2 {

DirListJob::DirListJob(const FilePath& path, Flags flags): dir_path{path} {
}

void DirListJob::run() {
_retry:
    GError* err = NULL;
    GFile* gf = dir_path.gfile().get();
    GObjectPtr<GFileInfo> dir_inf{g_file_query_info(gf, gfile_info_query_attribs, G_FILE_QUERY_INFO_NONE, cancellable_.get(), &err), false};
    if(!dir_inf) {
#if 0
        FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MODERATE);
        g_error_free(err);
        if(act == FM_JOB_RETRY) {
            err = NULL;
            goto _retry;
        }
#endif
        return;
    }

    if(g_file_info_get_file_type(dir_inf.get()) != G_FILE_TYPE_DIRECTORY) {
#if 0
        char* path_str = fm_path_to_str(job->dir_path);
        err = g_error_new(G_IO_ERROR, G_IO_ERROR_NOT_DIRECTORY,
                          _("The specified directory '%s' is not valid"),
                          path_str);
        fm_job_emit_error(fmjob, err, FM_JOB_ERROR_CRITICAL);
        g_free(path_str);
        g_error_free(err);
#endif
        return;
    }
    else {
        std::lock_guard<std::mutex> lock{mutex_};
        dir_fi = std::make_shared<FileInfo>(dir_inf);
    }

    FileInfoList foundFiles;
    /* check if FS is R/O and set attr. into inf */
    // FIXME:  _fm_file_info_job_update_fs_readonly(gf, inf, NULL, NULL);
    GObjectPtr<GFileEnumerator> enu = GObjectPtr<GFileEnumerator>{
            g_file_enumerate_children(gf, gfile_info_query_attribs,
                                      G_FILE_QUERY_INFO_NONE, cancellable_.get(), &err),
            false};
    if(enu) {
        while(!isCancelled()) {
            GObjectPtr<GFileInfo> inf{g_file_enumerator_next_file(enu.get(), cancellable_.get(), &err), false};
            if(inf) {
#if 0
                FmPath* dir, *sub;
                GFile* child;
                if(G_UNLIKELY(job->flags & FM_DIR_LIST_JOB_DIR_ONLY)) {
                    /* FIXME: handle symlinks */
                    if(g_file_info_get_file_type(inf) != G_FILE_TYPE_DIRECTORY) {
                        g_object_unref(inf);
                        continue;
                    }
                }

                /* virtual folders may return children not within them */
                dir = fm_path_new_for_gfile(g_file_enumerator_get_container(enu));
                if(fm_path_equal(job->dir_path, dir)) {
                    sub = fm_path_new_child(job->dir_path, g_file_info_get_name(inf));
                }
                else {
                    sub = fm_path_new_child(dir, g_file_info_get_name(inf));
                }
                child = g_file_get_child(g_file_enumerator_get_container(enu),
                                         g_file_info_get_name(inf));
                if(g_file_info_get_file_type(inf) == G_FILE_TYPE_DIRECTORY)
                    /* for dir: check if its FS is R/O and set attr. into inf */
                {
                    _fm_file_info_job_update_fs_readonly(child, inf, NULL, NULL);
                }
                fi = fm_file_info_new_from_g_file_data(child, inf, sub);
                fm_path_unref(sub);
                fm_path_unref(dir);
                g_object_unref(child);
#endif
                auto fileInfo = std::make_shared<FileInfo>(inf);
                if(emit_files_found) {
                    // Q_EMIT filesFound();
                }
                foundFiles.push_back(std::move(fileInfo));
            }
            else {
#if 0
                if(err) {
                    FmJobErrorAction act = fm_job_emit_error(fmjob, err, FM_JOB_ERROR_MILD);
                    g_error_free(err);
                    /* FM_JOB_RETRY is not supported. */
                    if(act == FM_JOB_ABORT) {
                        fm_job_cancel(fmjob);
                    }
                }
#endif
                /* otherwise it's EOL */
                break;
            }
        }
        g_file_enumerator_close(enu.get(), cancellable_.get(), &err);
    }
    else {
        /* FIXME:
        fm_job_emit_error(fmjob, err, FM_JOB_ERROR_CRITICAL);
        g_error_free(err);
        */
    }

    if(!foundFiles.empty()) {
        std::lock_guard<std::mutex> lock{mutex_};
        files_.swap(foundFiles);
    }

    Q_EMIT finished();
}

#if 0
//FIXME: incremental..

static gboolean emit_found_files(gpointer user_data) {
    /* this callback is called from the main thread */
    FmDirListJob* job = FM_DIR_LIST_JOB(user_data);
    /* g_print("emit_found_files: %d\n", g_slist_length(job->files_to_add)); */

    if(g_source_is_destroyed(g_main_current_source())) {
        return FALSE;
    }
    g_signal_emit(job, signals[FILES_FOUND], 0, job->files_to_add);
    g_slist_free_full(job->files_to_add, (GDestroyNotify)fm_file_info_unref);
    job->files_to_add = NULL;
    job->delay_add_files_handler = 0;
    return FALSE;
}

static gpointer queue_add_file(FmJob* fmjob, gpointer user_data) {
    FmDirListJob* job = FM_DIR_LIST_JOB(fmjob);
    FmFileInfo* file = FM_FILE_INFO(user_data);
    /* this callback is called from the main thread */
    /* g_print("queue_add_file: %s\n", fm_file_info_get_disp_name(file)); */
    job->files_to_add = g_slist_prepend(job->files_to_add, fm_file_info_ref(file));
    if(job->delay_add_files_handler == 0)
        job->delay_add_files_handler = g_timeout_add_seconds_full(G_PRIORITY_LOW,
                                       1, emit_found_files, g_object_ref(job), g_object_unref);
    return NULL;
}

void fm_dir_list_job_add_found_file(FmDirListJob* job, FmFileInfo* file) {
    fm_file_info_list_push_tail(job->files, file);
    if(G_UNLIKELY(job->emit_files_found)) {
        fm_job_call_main_thread(FM_JOB(job), queue_add_file, file);
    }
}
#endif

} // namespace Fm2
