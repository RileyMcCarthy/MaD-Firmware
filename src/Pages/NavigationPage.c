#include "NavigationPage.h"
#include "Images.h"
#include "StatusPage.h"
#include "ManualPage.h"
/**
 * @brief Navigation page shows all available pages to access.
 * 
 */

/**
 * @brief Number of buttons on the page
 * 
 */
#define BUTTONCOUNT 3

/**
 * @brief Enum for name of buttons on page
 * 
 */

/**
 * @brief Callback method when Button is pressed
 * 
 * @param button Button that was pressed
 */
void check_buttons(NavigationPage *page)
{
    if (display_check_buttons(page->display, buttons, BUTTONCOUNT) > 0)
    {
        for (int i = 0; i < BUTTONCOUNT; i++)
        {
            if (page->buttons[i].pressed)
            {
                switch (page->buttons[i].name)
                {
                case PAGE_STATUS:
                    complete = true;
                    newPage = PAGE_STATUS;
                    break;
                case PAGE_MANUAL:
                    complete = true;
                    newPage = PAGE_MANUAL;
                    break;
                case PAGE_AUTOMATIC:
                    complete = true;
                    newPage = PAGE_AUTOMATIC;
                    break;
                default:
                    break;
                }
            }
        }
    }
}

NavigationPage *navigation_page_create(Display *display)
{
    NavigationPage *page = malloc(sizeof(NavigationPage));
    page->display = display;
    page->complete = false;
    return page;
}

void navigation_page_destroy(NavigationPage *page)
{
    free(page);
}

/**
 * @brief Starts navigation page
 * 
 */
Pages navigation_page_run(NavigationPage *page)
{
    printf("Starting navigation page\n");
    page->complete = false;

    display_draw_square_fill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BACKCOLOR);
    display_set_text_parameter1(RA8876_SELECT_INTERNAL_CGROM, RA8876_CHAR_HEIGHT_32, RA8876_SELECT_8859_1);
    display_set_text_parameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE, RA8876_TEXT_WIDTH_ENLARGEMENT_X2, RA8876_TEXT_HEIGHT_ENLARGEMENT_X2);
    display_text_color(MAINTEXTCOLOR, BACKCOLOR);

    int buttonSize = 200;

    char buf[50];
    strcpy(buf, "Navigation Menu");
    int titlex = SCREEN_WIDTH / 2 - strlen(buf) * 16;
    int titley = 30;
    display_draw_string(titlex, titley, buf);
    printf("creating buttons\n");
    Button buttons[BUTTONCOUNT];
    buttons[0].name = PAGE_STATUS;
    buttons[0].xmin = SCREEN_WIDTH / 6 - buttonSize / 2;
    buttons[0].xmax = buttons[0].xmin + buttonSize;
    buttons[0].ymin = 120;
    buttons[0].ymax = buttons[0].ymin + buttonSize;
    buttons[0].pressed = false;
    buttons[0].lastPress = 0;

    buttons[1].name = PAGE_MANUAL;
    buttons[1].xmin = buttons[0].xmax + 30;
    buttons[1].xmax = buttons[1].xmin + buttonSize;
    buttons[1].ymin = 120;
    buttons[1].ymax = buttons[1].ymin + buttonSize;
    buttons[1].pressed = false;
    buttons[1].lastPress = 0;

    buttons[2].name = PAGE_AUTOMATIC;
    buttons[2].xmin = buttons[1].xmax + 30;
    buttons[2].xmax = buttons[2].xmin + buttonSize;
    buttons[2].ymin = 120;
    buttons[2].ymax = buttons[2].ymin + buttonSize;
    buttons[2].pressed = false;
    buttons[2].lastPress = 0;

    display_set_text_parameter1(RA8876_SELECT_INTERNAL_CGROM, RA8876_CHAR_HEIGHT_32, RA8876_SELECT_8859_1);
    display_set_text_parameter2(RA8876_TEXT_FULL_ALIGN_DISABLE, RA8876_TEXT_CHROMA_KEY_DISABLE, RA8876_TEXT_WIDTH_ENLARGEMENT_X1, RA8876_TEXT_HEIGHT_ENLARGEMENT_X1);

    display_draw_square_fill(buttons[0].xmin, buttons[0].ymin, buttons[0].xmax, buttons[0].ymax, MAINCOLOR);
    display_text_color(MAINTEXTCOLOR, MAINCOLOR);
    strcpy(buf, "Status");
    display_draw_string(buttons[0].xmin + buttonSize / 2 - strlen(buf) * 8, buttons[0].ymin + buttonSize / 2 - 12, buf);

    display_bte_memory_copy_image(manualImg, buttons[1].xmin, buttons[1].ymin);

    display_bte_memory_copy_image(automaticImg, buttons[2].xmin, buttons[2].ymin);

    while (!complete)
    {
        check_buttons(buttons);
        // clock.render();
    }
    return newPage;
}