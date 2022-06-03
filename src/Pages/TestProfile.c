#include "TestProfile.h"
#include "Explorer.h"
#include "motionPlanning.h"

#define BUTTONCOUNT 3
#define BUTTON_NAVIGATION 0
#define BUTTON_NEW 1
#define BUTTON_OPEN 2

#define BUTTON_QUARTET_COUNT 3
#define BUTTON_QUARTET_NAME 0
#define BUTTON_QUARTET_FUNC 1
#define BUTTON_QUARTET_DWELL 2

#define BUTTON_SET_COUNT 3
#define BUTTON_SET_NAME 0
#define BUTTON_SET_EXECUTIONS 1
#define BUTTON_SET_QUARTETS 2

#define PROFILE_TYPES 4
#define PROFILE_QUARTET 0
#define PROFILE_SET 1
#define PROFILE_MOTION 2
#define PROFILE_TEST 3

static bool complete;

static void button_navigation(int id, void *arg)
{
    TestProfilePage *page = (TestProfilePage *)arg;
    complete = true;
}

static void button_open(int id, void *arg)
{
    char *extension[] = {".QRT", ".SET", ".MOT", ".TST"};
    TestProfilePage *page = (TestProfilePage *)arg;

    Explorer *explorer = explorer_create(page->display, 100, 100, EXPLORER_MODE_FILE, "/sd");
    char *filepath = explorer_run(explorer);
    explorer_destroy(explorer);
    if (filepath == NULL)
    {
        return;
    }

    char *filename = strrchr(filepath, '/');
    free(page->filename);
    page->filename = (char *)malloc(strlen(filename) + 1);
    strcpy(page->filename, filename);

    free(page->path);
    page->path = malloc(strlen(filepath) - strlen(filename) + 1);
    strncpy(page->path, filepath, strlen(filepath) - strlen(filename));

    for (int i = 0; i < PROFILE_TYPES; i++)
    {
        char *ext = filepath;
        ext += strlen(filepath) - strlen(extension[i]);
        printf("%s\n", ext);
        if (strcmp(ext, extension[i]) == 0)
        {
            printf("Found mode:%d\n", i);
            page->mode = i;
            break;
        }
    }
    printf("Mode:%d\n", page->mode);
    free(filepath);
    free(page->quartet);
    free(page->set);
    free(page->profile);
    free(page->test);

    switch (page->mode)
    {
    case PROFILE_QUARTET:
    {
        page->quartet = (MotionQuartet *)malloc(sizeof(MotionQuartet));
        printf("Quartet from json\n");
        json_to_motion_quartet(filepath, page->quartet);
        break;
    }
    case PROFILE_SET:
    {
        page->set = (MotionSet *)malloc(sizeof(MotionSet));
        json_to_motion_set(filepath, page->set);
        break;
    }
    case PROFILE_MOTION:
    {
        page->profile = (MotionProfile *)malloc(sizeof(MotionProfile));
        json_to_motion_profile(filepath, page->profile);
        break;
    }
    case PROFILE_TEST:
    {
        page->test = (TestProfile *)malloc(sizeof(TestProfile));
        json_to_test_profile(filepath, page->test);
        break;
    }
    }
    free(filepath);
}

static void button_new(int id, void *arg)
{
    TestProfilePage *page = (TestProfilePage *)arg;

    char *optionNames[] = {"Quartet", "Set", "Motion", "Test"};
    char *extension[] = {".QRT", ".SET", ".MOT", ".TST"};
    int newmode = selection_run(page->display, 100, 100, optionNames, PROFILE_TYPES);

    Keyboard *keyboard = keyboard_create(page->display, page->images);
    if (keyboard == NULL)
    {
        printf("TestProfile Button new Keyboard could not allocate memory\n");
    }
    char *filename = keyboard_get_input(keyboard, "Enter file name: ");
    keyboard_destroy(keyboard);

    if (filename == NULL)
    {
        return;
    }

    page->filename = (char *)realloc(page->filename, strlen(filename) + strlen(extension[page->mode]) + 3);
    if (page->filename == NULL)
    {
        printf("Testprofile.c page->filename could not allocate memory\n");
    }
    strcpy(page->filename, filename);
    strcat(page->filename, extension[newmode]);
    free(filename);

    Explorer *explorer = explorer_create(page->display, 100, 100, EXPLORER_MODE_DIRECTORY, "/sd");
    if (explorer == NULL)
    {
        printf("Testprofile.c explorer could not allocate memory\n");
    }
    free(page->path);
    char *newpath = explorer_run(explorer);
    explorer_destroy(explorer);

    if (newpath == NULL)
    {
        return;
    }
    page->mode = newmode;
    page->path = newpath;

    free(page->quartet);
    free(page->set);
    free(page->profile);
    free(page->test);
}

static void button_save(int id, void *arg)
{
    TestProfilePage *page = (TestProfilePage *)arg;
    char *filepath = (char *)malloc(strlen(page->path) + strlen(page->filename) + 2);
    if (filepath == NULL)
    {
        printf("Testprofile.c filepath could not allocate memory\n");
    }
    strcpy(filepath, page->path);
    strcat(filepath, "/");
    strcat(filepath, page->filename);
    motion_quartet_to_json(page->quartet, filepath);
    if (page->quartet == NULL)
    {
        printf("Testprofile.c page->quartet could not allocate memory\n");
    }
    free(filepath);
}

static void button_simulate(int id, void *arg)
{
    TestProfilePage *page = (TestProfilePage *)arg;
    switch (page->mode)
    {
    case PROFILE_QUARTET:
    {
        motion_quartet_simulate(page->quartet);
        break;
    }
    case PROFILE_SET:
    {
        motion_set_simulate(page->set);
        break;
    }
    case PROFILE_MOTION:
    {
        motion_profile_simulate(page->profile);
        break;
    }
    case PROFILE_TEST:
    {
        test_profile_simulate(page->test);
        break;
    }
    }
    double max_velocity_rpm = 12.21 * 1 * (48 + 3)(48 + 3) / 16; //((float)value / 1000.0) * (60.0 / 80.0);              // um/s to rpm
    double max_acceleration_rpms = 30 * 635.78 * 1;
    double v_max = (0.08) * (max_velocity_rpm) / (60.0);
    double a_max = (0.08) * (max_acceleration_rpms) / (60.0);
    SetPoint *setpoint = create_empty_setpoint();
    double t = 0;
    while (abs(d_max - setpoint->x) > error)
    {
        // simulate_profile(setpoint, t, d_max, v_max, a_max, sigmoid, d_max, sr, error);
        printf("%f,%f,%f,%f\n", t + 0.0386, setpoint->x, setpoint->v, setpoint->a);
        t += error * 4;
    }
}
static void button_quartet(int id, void *arg)
{
    printf("BUTTON_QUARTET_id:%d\n", id);
    TestProfilePage *page = (TestProfilePage *)arg;
    switch (id)
    {
    case BUTTON_QUARTET_NAME:
    {
        Keyboard *keyboard = keyboard_create(page->display, page->images);
        char *filename = keyboard_get_input(keyboard, "Enter file name: ");
        keyboard_destroy(keyboard);
        free(page->quartet->name);
        strcpy(page->quartet->name, page->path);
        strcat(page->quartet->name, "/");
        strcat(page->quartet->name, filename);
        strcat(page->quartet->name, ".qrt");
        free(filename);
        break;
    }
    case BUTTON_QUARTET_FUNC:
    {
        char *optionNames[FUNCTION_COUNT];
        for (int i = 0; i < FUNCTION_COUNT; i++)
        {
            FunctionInfo info;
            get_function_info(&info, i);
            printf("function:%d,name:%s\n", i, info.name);
            optionNames[i] = (char *)malloc(strlen(info.name) + 1);
            strcpy(optionNames[i], info.name);
        }
        page->quartet->function = selection_run(page->display, 300, 150, optionNames, FUNCTION_COUNT);
        for (int i = 0; i < FUNCTION_COUNT; i++)
        {
            free(optionNames[i]);
        }
        break;
    }
    case BUTTON_QUARTET_DWELL:
    {
        Keyboard *keyboard = keyboard_create(page->display, page->images);
        char *dwell = keyboard_get_input(keyboard, "Dwell: ");
        keyboard_destroy(keyboard);
        page->quartet->dwell = atof(dwell);
        free(dwell);
        break;
    }
    default:
        break;
    }
}

static void button_quartet_parameters(int id, void *arg)
{
    TestProfilePage *page = (TestProfilePage *)arg;
    FunctionInfo info;
    get_function_info(&info, page->quartet->function);
    Keyboard *keyboard = keyboard_create(page->display, page->images);
    char *prompt = (char *)malloc(strlen(info.args[id]) + strlen(": ") + 1);
    strcpy(prompt, info.args[id]);
    strcat(prompt, ": ");
    char *param = keyboard_get_input(keyboard, prompt);
    free(prompt);
    keyboard_destroy(keyboard);
    page->quartet->parameters[id] = atof(param);
    free(param);
}

static void button_set(int id, void *arg)
{
    TestProfilePage *page = (TestProfilePage *)arg;
    switch (id)
    {
    case BUTTON_SET_NAME:
    {
        Keyboard *keyboard = keyboard_create(page->display, page->images);
        char *filename = keyboard_get_input(keyboard, "Enter file name: ");
        keyboard_destroy(keyboard);
        free(page->set->name);
        strcpy(page->set->name, page->path);
        strcat(page->set->name, "/");
        strcat(page->set->name, filename);
        strcat(page->set->name, ".set");
        free(filename);
        break;
    }
    case BUTTON_SET_EXECUTIONS:
    {
        Keyboard *keyboard = keyboard_create(page->display, page->images);
        char *executions = keyboard_get_input(keyboard, "Executions: ");
        page->set->executions = atoi(executions);
        keyboard_destroy(keyboard);
        free(executions);
        break;
    }
    case BUTTON_SET_QUARTETS:
    {
        char *optionNames[] = {"Add", "Remove"};
        int newmode = selection_run(page->display, 100, 100, optionNames, 2);
        if (newmode == 0) // Add
        {
            Explorer *explorer = explorer_create(page->display, 100, 100, EXPLORER_MODE_FILE, "/sd");
            if (explorer == NULL)
            {
                printf("Testprofile.c explorer could not allocate memory\n");
            }

            char *newfile = explorer_run(explorer);
            explorer_destroy(explorer);

            if (newfile != NULL)
            {
                page->set->quartetCount++;
                json_to_motion_quartet(newfile, &(page->set->quartets[page->set->quartetCount - 1]));
            }
            else
            {
                return;
            }
        }
        else if (newmode == 1) // Remove
        {
            char **optionNames = (char **)malloc(sizeof(char *) * page->set->quartetCount);
            for (int i = 0; i < page->set->quartetCount; i++)
            {
                optionNames[i] = (char *)malloc(strlen(page->set->quartets[i].name) + 1);
                strcpy(optionNames[i], page->set->quartets[i]);
            }
            int index = selection_run(page->display, 100, 100, optionNames, 2);

            // Free option names
            for (int i = 0; i < page->set->quartetCount; i++)
            {
                free(optionNames[i]);
            }
            free(optionNames);

            // Remove index for quartets
            free(&(page->set->quartets[index]));
            for (int i = index; i < page->set->quartetCount - 1; i++)
            {
                page->set->quartets[i] = page->set->quartets[i + 1];
            }
            page->set->quartetCount--;
        }
        else
        {
            printf("something is wrong:%d\n", newmode);
        }
    }
    }
}

void test_profile_page_init(TestProfilePage *page, Display *display, Images *images)
{
    page->display = display;
    page->images = images;
    page->mode = -1;
    page->quartet = NULL;
    page->set = NULL;
    page->profile = NULL;
    page->test = NULL;
    page->filename = NULL;
}

void test_profile_page_destroy(TestProfilePage *page)
{
    free(page);
}
void test_profile_page_run(TestProfilePage *page)
{
    complete = false;
    printf("Test profile page running\n");
    int padding = 20;

    // Create Background
    Module *root = module_create(NULL);
    Module *background = module_create(root);
    module_set_rectangle_circle(background, SCREEN_WIDTH, SCREEN_HEIGHT);
    module_set_position(background, 0, 0);
    module_set_padding(background, padding, padding);
    module_set_color(background, BACKCOLOR, BACKCOLOR);

    // Create edit window
    Module *editWindow = module_create(background);
    module_set_rectangle_circle(editWindow, SCREEN_WIDTH / 3, 0);
    module_fit_height(editWindow);
    module_set_padding(editWindow, padding, padding);
    module_set_color(editWindow, MAINCOLOR, BACKCOLOR);
    module_align_inner_left(editWindow);
    module_align_inner_top(editWindow);

    // Create Open Button
    Module *openButton = module_create(editWindow);
    module_set_rectangle_circle(openButton, 100, 50);
    module_set_color(openButton, COLOR65K_LIGHTGREEN, COLOR65K_LIGHTGREEN);
    module_set_padding(openButton, padding, padding);
    module_align_space_even(openButton, 1, 2);
    module_align_inner_top(openButton);
    module_touch_callback(openButton, button_open, 0);

    // Create Open Text
    Module *openText = module_create(openButton);
    module_set_text(openText, "Open");
    module_set_font(openText, -1);
    module_align_center(openText);
    module_align_middle(openText);
    module_set_color(openText, SECONDARYTEXTCOLOR, openText->parent->foregroundColor);

    // Create New Button
    Module *newButton = module_create(editWindow);
    module_copy(newButton, openButton);
    module_align_space_even(newButton, 2, 2);
    module_touch_callback(newButton, button_new, 0);

    // Create New Text
    Module *newText = module_create(newButton);
    module_copy(newText, openText);
    module_set_text(newText, "New");
    module_set_font(newText, -1);
    module_align_center(newText);

    // Create Save Button
    Module *saveButton = module_create(editWindow);
    module_copy(saveButton, openButton);
    module_align_center(saveButton);
    module_align_inner_bottom(saveButton);
    module_touch_callback(saveButton, button_save, 0);

    // Create New Text
    Module *saveText = module_create(saveButton);
    module_copy(saveText, openText);
    module_set_text(saveText, "Save");
    module_set_font(saveText, -1);
    module_align_center(saveText);
    module_align_middle(saveText);

    // Create navigation button
    Module *navigationButton = module_create(background);
    module_set_image(navigationButton, &(page->images->navigationImage));
    module_align_inner_top(navigationButton);
    module_align_inner_right(navigationButton);
    module_touch_callback(navigationButton, button_navigation, 0);

    // Create edit window title
    Module *editWindowTitle = module_create(editWindow);
    module_set_padding(editWindowTitle, padding, padding / 2);
    module_set_text(editWindowTitle, "Edit");
    module_set_font(editWindowTitle, RA8876_CHAR_HEIGHT_32);
    module_set_color(editWindowTitle, SECONDARYTEXTCOLOR, MAINCOLOR);
    module_align_below(editWindowTitle, newButton);
    module_align_center(editWindowTitle);
    module_add_underline(editWindowTitle);

    Module *graph1 = module_create(background);
    module_set_padding(graph1, 0, 0);
    module_set_size(graph1, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - padding * 3);
    module_align_right(graph1, editWindow);
    module_align_inner_top(graph1);
    module_set_color(graph1, graph1->parent->foregroundColor, graph1->parent->backgroundColor);
    module_set_graph(graph1, "Position Vs. Time", "mm");
    module_graph_set_range(graph1, 5, -5);

    module_draw(page->display, root);
    int lastMode = -1;
    while (!complete)
    {
        printf("Test profile page running\n");
        Module *subroot = module_create(editWindow);
        module_align_below(subroot, editWindowTitle);
        module_fit_below(subroot, editWindowTitle);
        module_align_inner_left(subroot);
        module_set_color(subroot, subroot->parent->foregroundColor, subroot->parent->backgroundColor);
        module_set_padding(subroot, 0, 0);
        printf("Mode:%d\n", page->mode);
        switch (page->mode)
        {
        case PROFILE_QUARTET:
        {
            if (page->quartet == NULL)
            {
                page->quartet = (MotionQuartet *)malloc(sizeof(MotionQuartet));
                strcpy(page->quartet->name, page->filename);
            }

            char buf[50];
            printf("Quartet\n");
            module_set_text(editWindowTitle, "Edit Quartet");
            module_set_font(editWindowTitle, RA8876_CHAR_HEIGHT_32);
            module_align_center(editWindowTitle);
            module_add_underline(editWindowTitle);

            Module *nameModule = module_create(subroot);
            sprintf(buf, "Name: %s", page->quartet->name);
            module_set_text(nameModule, buf);
            module_set_font(nameModule, RA8876_CHAR_HEIGHT_24);
            module_align_inner_left(nameModule);
            module_align_inner_top(nameModule);
            module_set_color(nameModule, SECONDARYTEXTCOLOR, nameModule->parent->foregroundColor);
            module_touch_callback(nameModule, button_quartet, BUTTON_QUARTET_NAME);

            Module *funcModule = module_create(subroot);
            module_copy(funcModule, nameModule);

            FunctionInfo info;
            get_function_info(&info, page->quartet->function);
            sprintf(buf, "Function: %s", info.name);
            module_set_text(funcModule, buf);
            module_set_font(funcModule, RA8876_CHAR_HEIGHT_24);
            module_align_below(funcModule, nameModule);
            module_touch_callback(funcModule, button_quartet, BUTTON_QUARTET_FUNC);

            int params = info.args_count;
            printf("info->args_count:%d\n", info.args_count);
            Module *below = funcModule;
            for (int i = 0; i < params; i++) //[distance, strain rate, error]
            {
                Module *paramModule = module_create(subroot);
                module_copy(paramModule, nameModule);
                sprintf(buf, "%s: %0.3f", info.args[i], page->quartet->parameters[i]);
                module_set_text(paramModule, buf);
                module_set_font(paramModule, RA8876_CHAR_HEIGHT_24);
                module_align_below(paramModule, below);
                module_touch_callback(paramModule, button_quartet_parameters, i);
                below = paramModule;
            }

            printf("running quartet\n");
            /*RunMotionProfile *run = get_run_motion_profile();

            double t = 0;
            // Find t_max
            long startTime = _getms();
            double d_max = 0;
            double d_min = 0;
            while (!run->quartetComplete)
            {
                double distance = position_quartet(t, run, page->quartet);
                printf("distance:%f\n", distance);
                t += 0.01;
                if (distance > d_max)
                {
                    d_max = distance;
                }
                if (distance < d_min)
                {
                    d_min = distance;
                }
                if ((_getms() - startTime) > 1000) // timeout
                {
                    printf("Timeout\n");
                    break;
                }
            }
            destroy_run_motion_profile(run);

            module_graph_set_range(graph1, 10, -10);
            run = get_run_motion_profile();
            printf("t_max:%f\n", t);
            int dataPoints = module_graph_get_max_data(graph1);
            printf("dataPoints:%d\n", dataPoints);
            double dt = t * 1000 / (double)dataPoints; // Get interval for graph
            for (int i = 0; i < dataPoints; i++)
            {
                double position = position_quartet(i * dt / 1000.0, run, page->quartet);
                module_graph_insert(graph1, position);
                module_draw(page->display, graph1);
                // printf("graph1Data%f,%f\n", position, i * dt);
            }
            printf("total time:%f\n", t);
            destroy_run_motion_profile(run);*/

            Module *dwellModule = module_create(subroot);
            module_copy(dwellModule, nameModule);
            sprintf(buf, "Dwell (ms): %0.3f", page->quartet->dwell);
            module_set_text(dwellModule, buf);
            module_set_font(dwellModule, RA8876_CHAR_HEIGHT_24);
            module_align_below(dwellModule, below);
            module_touch_callback(dwellModule, button_quartet, BUTTON_QUARTET_DWELL);
            break;
        }
        case PROFILE_SET:
        {
            /*    char *name;              // Motion set Filename, user defined
    int number;              // Motion set number, autogenerated as incrment in file system
    char *type;              // descriptive purpose of the set (ie. precondition, test, etc)
    int executions;          // Number of times to execute the motion set
    int quartetCount;        // Number of motion quartets in the set
    MotionQuartet *quartets; // List of quartets to execute (max 10)*/
            if (page->set == NULL)
            {
                page->set = (MotionSet *)malloc(sizeof(MotionSet));
                strcpy(page->set->name, page->filename);
            }
            module_set_text(editWindowTitle, "Edit Set");
            module_set_font(editWindowTitle, RA8876_CHAR_HEIGHT_32);
            module_align_center(editWindowTitle);
            module_add_underline(editWindowTitle);

            char buf[50];
            Module *nameModule = module_create(subroot);
            sprintf(buf, "Name: %s", page->set->name);
            module_set_text(nameModule, buf);
            module_set_font(nameModule, RA8876_CHAR_HEIGHT_24);
            module_align_inner_left(nameModule);
            module_align_inner_top(nameModule);
            module_set_color(nameModule, SECONDARYTEXTCOLOR, nameModule->parent->foregroundColor);
            module_touch_callback(nameModule, button_set, BUTTON_SET_NAME);

            Module *funcModule = module_create(subroot);
            module_copy(funcModule, nameModule);
            break;
        }
        case PROFILE_MOTION:
        {
            break;
        }
        case PROFILE_TEST:
        {
            break;
        }
        default:
            break;
        }

        module_draw(page->display, subroot);
        do
        {
            display_update_touch(page->display);
        } while (module_touch_check(root, page->display, page) == 0);
        module_trim(subroot); // Remove subroot from tree
        module_destroy(subroot);
    }
    module_destroy(root);
}
