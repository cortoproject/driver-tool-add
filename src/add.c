
#include <driver/tool/add/add.h>

/* This command uses parson instead of corto serialization/deserialization to
 * ensure that any members not part of the corto package type are preserved. */

int cortomain(int argc, char *argv[]) {
    char *pkg = NULL;
    JSON_Value *json = NULL;

    if (argc == 2) {
        pkg = argv[1];
    } else if (argc == 3) {
        if (corto_chdir(argv[1])) {
            corto_throw(NULL);
            goto error;
        }
        pkg = argv[2];
    } else {
        corto_throw("invalid number of arguments");
        goto error;
    }

    if (pkg[0] == '/') {
        pkg ++;
    }

    if (!strlen(pkg)) {
        corto_throw("invalid package identifier");
        goto error;
    }

    if (!corto_file_test("project.json")) {
        corto_throw("directory '%s' does not contain a corto project",
            corto_cwd());
        goto error;
    }

    json = json_parse_file("project.json");
    if (!json) {
        corto_throw("failed to parse 'project.json'");
        goto error;
    }

    JSON_Object *jsonObject = json_value_get_object(json);
    if (!jsonObject) {
        corto_throw("invalid JSON in 'project.json': expected object");
        goto error;
    }

    JSON_Object *json_m_value = NULL;
    if (json_object_has_value(jsonObject, "value")) {
        json_m_value = json_object_get_object(jsonObject, "value");
        if (!json_m_value) {
            /* If JSON has a value member, but wasn't resolved by the previous
             * call, it is not of an object type */
            corto_throw("invalid JSON: expected 'value' to be a JSON object");
            goto error;
        }
    } else {
        /* This is not an error, a project file can leave all project
         * settings to defaults. Add a value member. */
        json_object_set_value(
            jsonObject, "value", json_value_init_object());
        json_m_value = json_object_get_object(jsonObject, "value");
    }

    JSON_Array *json_m_use = NULL;
    if (json_object_has_value(json_m_value, "use")) {
        json_m_use = json_object_get_array(json_m_value, "use");
        if (!json_m_use) {
            /* If JSON has a use member, but wasn't resolved by the previous
             * call, it is not of an array type */
            corto_throw("invalid JSON: expected 'value.use' to be a JSON array");
            goto error;
        }
    }  else {
        json_object_set_value(
            json_m_value, "use", json_value_init_array());
        json_m_use = json_object_get_array(json_m_value, "use");
    }

    /* Add package if it wasn't already added */
    int i;
    for (i = 0; i < json_array_get_count(json_m_use); i ++) {
        const char *use = json_array_get_string(json_m_use, i);
        if (use[0] == '/') {
            use ++;
        }

        if (!strcmp(use, pkg)) {
            /* Package is already added */
            break;
        }
    }

    /* If loop finished, we didn't find the package */
    if (i == json_array_get_count(json_m_use)) {
        json_array_append_string(json_m_use, pkg);
    }

    char *escapedStr = json_serialize_to_string_pretty(json);

    /* Parson escapes '/' which doesn't look nice in the project.json file.
     * Until this becomes an optional feature in parson, replace \/ with / */
    char *str = strreplace(escapedStr, "\\/", "/");
    json_value_free(json);
    free(escapedStr);

    /* Write to file */
    FILE *f = fopen("project.json", "w");
    fprintf(f, "%s\n", str);
    fclose(f);
    free(str);

    return 0;
error:
    if (json) {
        json_value_free(json);
    }
    return -1;
}
