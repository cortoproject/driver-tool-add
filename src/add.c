/* Copyright (c) 2010-2017 the corto developers
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <driver/tool/add/add.h>
#include <corto/argparse/argparse.h>

corto_string corto_tool_lookupPackage(corto_string str) {
    corto_string package = NULL;

    corto_object p = corto_resolve(NULL, str);

    if (!p) {
        if (!corto_locate(str, NULL, CORTO_LOCATION_LIB)) {
            corto_seterr("package '%s' not found", str);
            goto error;
        } else {
            package = str;
        }
    } else {
        if (!corto_instanceof(corto_package_o, p)) {
            corto_seterr("object '%s' is not a package", str);
            goto error;
        } else {
            package = corto_fullpath(NULL, p);
        }
        corto_release(p);
    }

    return package;
error:
    return NULL;
}

int cortomain(int argc, char *argv[]) {
    corto_ll silent, mute, nobuild, nocoverage, project, packages;
    corto_bool build = FALSE;
    CORTO_UNUSED(argc);

    corto_argdata *data = corto_argparse(
      argv,
      (corto_argdata[]){
        /* Ignore first argument */
        {"$0", NULL, NULL},
        {"--silent", &silent, NULL},
        {"--mute", &mute, NULL},
        {"--nobuild", &nobuild, NULL},
        {"--nocoverage", &nocoverage, NULL},

        /* Match at most one project directory */
        {"$?*", &project, NULL},

        /* At least one package must be specified */
        {"$+*", &packages, NULL},
        {NULL}
      }
    );

    if (!data) {
        goto error;
    }

    /* Move to project directory */
    if (project) {
        if (corto_chdir(corto_ll_get(project, 0))) {
            goto error;
        }
    }

    /* Load packages */
    if (packages) {
        corto_iter iter = corto_ll_iter(packages);
        while (corto_iter_hasNext(&iter)) {
            /* Test whether package exists */
            corto_string package =
                corto_tool_lookupPackage(corto_iter_next(&iter));
            if (!package) {
                goto error;
            }

            /* Use fully scoped name from here */
            if (!corto_loadRequiresPackage(package)) {
                corto_mkdir(".corto");
                corto_file f = corto_fileAppend(".corto/packages.txt");
                if (!f) {
                    corto_seterr("failed to open .corto/packages.txt (check permissions)");
                    goto error;
                }

                /* Only add fully scoped names to package file */
                if (*package != '/') {
                    fprintf(corto_fileGet(f), "/%s\n", package);
                } else {
                    fprintf(corto_fileGet(f), "%s\n", package);
                }
                corto_fileClose(f);
                build = TRUE;

                if (!silent) {
                    printf("package '%s' added to project\n", package);
                }
            } else {
                if (!silent) {
                    printf("package '%s' is already added to the project\n", package);
                }
            }

        }
    }

    if (build && !nobuild) {
        corto_load("driver/tool/build", 3, (char*[]){
          "build",
          silent ? "--silent" : "",
          mute ? "--mute" : "",
          nocoverage ? "--nocoverage" : "",
          NULL
        });
    }

    corto_argclean(data);

    return 0;
error:
    corto_error("add: %s", corto_lasterr());
    return -1;
}

