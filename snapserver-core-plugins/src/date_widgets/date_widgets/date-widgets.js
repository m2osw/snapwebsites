/** @preserve
 * Name: date-widgets
 * Version: 0.0.1.40
 * Browsers: all
 * Depends: editor (>= 0.0.3.640)
 * Copyright: Copyright 2013-2017 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
 */

//
// Inline "command line" parameters for the Google Closure Compiler
// https://developers.google.com/closure/compiler/docs/js-for-compiler
//
// See output of:
//    java -jar .../google-js-compiler/compiler.jar --help
//
// ==ClosureCompiler==
// @compilation_level ADVANCED_OPTIMIZATIONS
// @externs $CLOSURE_COMPILER/contrib/externs/jquery-1.9.js
// @externs plugins/output/externs/jquery-extensions.js
// @js plugins/output/output.js
// @js plugins/output/popup.js
// @js plugins/server_access/server-access.js
// @js plugins/listener/listener.js
// @js plugins/editor/editor.js
// ==/ClosureCompiler==
//
// This is not required and it may not exist at the time you run the
// JS compiler against this file (it gets generated)
// --js plugins/mimetype/mimetype-basics.js
//

/*
 * JSLint options are defined online at:
 *    http://www.jshint.com/docs/options/
 */
/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false, FileReader: true, Blob: true */




/** \file
 * \brief The Date widgets for the Snap! Editor.
 *
 * This file defines a set of editor extensions to offer ways to
 * easily enter a date in a widget (i.e. dropdown calendar, verify
 * that the date is valid, etc.)
 *
 * \code
 *      .
 *      .
 *      .
 *      |
 *  +---+--------------------+  Inherit
 *  |                        |<----------------------+
 *  |  EditorWidgetType      |                       |
 *  |  (cannot instantiate)  |                       |
 *  +------------------------+                       |
 *      ^                                            |
 *      |  Inherit                                   |
 *      .                                            |       .
 *      .                                            |       .
 *      .                                            |       .
 *      |                                            |       |
 *  +---+------------------------------+             |   +---+----------------------+
 *  |                                  |             |   |                          |
 *  | EditorWidgetTypeLineEdit         |             |   | EditorWidgetTypeDropdown |
 *  |                                  |             |   |                          |
 *  +----------------------------------+             |   +--------------------------+
 *      ^                                            |       ^
 *      | Inherit                                    |       | Ref.
 *      |                                            |       |
 *  +---+------------------------------+         +---+-------+----------------------+
 *  |                                  |         |                                  |
 *  | EditorWidgetTypeDateEdit         |         | EditorWidgetTypeDropdownDateEdit |
 *  |                                  |         |                                  |
 *  +----------------------------------+         +----------------------------------+
 * \endcode
 */



/** \brief Editor widget type for Date widgets.
 *
 * This widget defines the "date-edit" in the editor forms. This is
 * an equivalent to the input tag of type text of a standard form.
 *
 * @return {!snapwebsites.EditorWidgetTypeDateEdit}
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeLineEdit}
 * @struct
 */
snapwebsites.EditorWidgetTypeDateEdit = function()
{
    snapwebsites.EditorWidgetTypeDateEdit.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeDateEdit, snapwebsites.EditorWidgetTypeLineEdit);


/** \brief Return "date-edit".
 *
 * Return the name of the widget type.
 *
 * @return {string} The name of the widget type.
 * @override
 */
snapwebsites.EditorWidgetTypeDateEdit.prototype.getType = function()
{
    return "date-edit";
};


/** \brief Initialize the widget.
 *
 * This function initializes the date-edit widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeDateEdit.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        c = editor_widget.getWidgetContent();

    // make sure the top window includes the necessary CSS too
    //
    // we do that early even if the calendar ends up never being used
    // because otherwise the size of the calendar ends up being incorrect
    // on the first time the widget gets opened
    //
    // another solution would be to have an on-load and open the calendar
    // after the CSS was loaded, but it makes things more complicated than
    // necessary (I think, at this time...)
    // See: http://stackoverflow.com/questions/3498647/jquery-loading-css-on-demand-callback-if-done/17858428#17858428
    // (i.e. to know once the CSS file is loaded, you need to program that
    // as there is no "onload" event on links.)
    //
    if(!jQuery("head link[rel='stylesheet'][href^='/css/editor/date-widgets_']", window.top.document).exists())
    {
        // it does not exist yet, make a copy of our iframe version
        jQuery("head link[rel='stylesheet'][href^='/css/editor/date-widgets_']").clone().appendTo(jQuery("head", window.top.document));
    }

    editor_widget.setData("calendarTimeoutID", NaN);

    snapwebsites.EditorWidgetTypeDateEdit.superClass_.initializeWidget.call(this, widget);

    c.focus(function()
        {
            that.openCalendar(editor_widget);
        })
     .blur(function()
        {
            that.startCalendarHide(editor_widget);
        })
     .click(function(e)
        {
            // if the user clicks the widget and the widget already
            // has focus, it will not attempt to reopen the calendar
            //
            if(!that.isCalendarOpen(editor_widget))
            {
                that.openCalendar(editor_widget);
            }
        })
     .on("cut paste input textinput", function()
        {
            if(that.isCalendarOpen(editor_widget))
            {
                that.startCalendarHide(editor_widget);
            }
        })
     .keydown(function(e)
        {
            switch(e.which)
            {
            case 40: // arrow down
                // always avoid default browser behavior
                e.preventDefault();

                if(!that.isCalendarOpen(editor_widget))
                {
                    // first arrow down just opens the calendar
                    that.openCalendar(editor_widget);
                }
                else
                {
                    that.createCalendar(editor_widget, "next-week");
                }
                break;

            case 38: // arrow up
                if(that.isCalendarOpen(editor_widget))
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    that.createCalendar(editor_widget, "previous-week");
                }
                break;

            case 37: // arrow left
                if(that.isCalendarOpen(editor_widget))
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    if(e.shiftKey)
                    {
                        that.createCalendar(editor_widget, "monday");
                    }
                    else
                    {
                        that.createCalendar(editor_widget, "previous-day");
                    }
                }
                break;

            case 39: // arrow right
                if(that.isCalendarOpen(editor_widget))
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    if(e.shiftKey)
                    {
                        that.createCalendar(editor_widget, "friday");
                    }
                    else
                    {
                        that.createCalendar(editor_widget, "next-day");
                    }
                }
                break;

            case 36: // home
                if(that.isCalendarOpen(editor_widget))
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    if(e.shiftKey)
                    {
                        that.createCalendar(editor_widget, "first-day-month");
                    }
                    else
                    {
                        that.createCalendar(editor_widget, "first-day-week");
                    }
                }
                break;

            case 35: // end
                if(that.isCalendarOpen(editor_widget))
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    if(e.shiftKey)
                    {
                        that.createCalendar(editor_widget, "last-day-month");
                    }
                    else
                    {
                        that.createCalendar(editor_widget, "last-day-week");
                    }
                }
                break;

            case 33: // page up
                if(that.isCalendarOpen(editor_widget))
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    if(e.shiftKey)
                    {
                        that.createCalendar(editor_widget, "previous-year");
                    }
                    else
                    {
                        that.createCalendar(editor_widget, "previous-month");
                    }
                }
                break;

            case 34: // page down
                if(that.isCalendarOpen(editor_widget))
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    if(e.shiftKey)
                    {
                        that.createCalendar(editor_widget, "next-year");
                    }
                    else
                    {
                        that.createCalendar(editor_widget, "next-month");
                    }
                }
                break;

            case 27: // escape (close, no changes)
                if(that.isCalendarOpen(editor_widget))
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    that.startCalendarHide(editor_widget);
                }
                break;

            case 13: // return (select)
                if(that.isCalendarOpen(editor_widget))
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    e.stopPropagation();

                    that.startCalendarHide(editor_widget);
                }
                break;

            }

            return true;
        });
};


/** \brief Check whether the calendar dropdown is open.
 *
 * This function check whether the calendar dropdown exists and whether it
 * is open. Note that the function returns false if the calendar dropdown
 * "visually" exists but the hideCalendar() function was called. This is
 * because as soon as we fade it out, it is considered deleted.
 *
 * Note that the startCalendarHide() may have been called and this function
 * will still return true since the dropdown is still available and the
 * caller still has a chance to call cancelCalendarHide() to prevent the
 * hide from happening.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget to check for
 *        the calendar dropdown window.
 *
 * @return {boolean}  true if the calendar is currently considered open.
 */
snapwebsites.EditorWidgetTypeDateEdit.prototype.isCalendarOpen = function(editor_widget)
{
    var calendar = editor_widget.getData("calendar");

    return calendar && calendar.is(":visible");
};


/** \brief Open the date widget calendar.
 *
 * This function generates a calendar, a rectangle representing a month.
 * The calendar includes a set of 4 buttons to move to the next/previous
 * month/year.
 *
 * Each day is also made a button which when clicked gets saved as the
 * selected date in the Date widget.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The editor widget
 *        that got focused.
 */
snapwebsites.EditorWidgetTypeDateEdit.prototype.openCalendar = function(editor_widget)
{
    var calendar,
        w = editor_widget.getWidget(),
        z;

    if(w.is(".read-only"))
    {
        return;
    }

    this.cancelCalendarHide(editor_widget);

    calendar = editor_widget.getData("calendar");
    if(!calendar)
    {
        calendar = this.createCalendar(editor_widget, "create");
    }

    // setup z-index each time we reopen the calendar
    // (reset itself first so we do not just +1 each time)
    calendar.css("z-index", 0);
    z = window.top.jQuery(".zordered").maxZIndex() + 1;
    calendar.css("z-index", z);

    calendar.fadeIn(300);
};


/** \brief Generate a calendar page.
 *
 * This function calculates a calendar page. It may be in replacement
 * from the existing calendar because the user clicked on a button
 * or used a key (i.e. Page Down or Next Month.)
 *
 * The function return a reference to the new calendar. Note that we
 * do not actually destroy the old calendar, we only replace the
 * table within if it already existed. That way the position is not
 * changed.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget receiving
 *        the new calendar.
 * @param {string} command  The command to apply to the existing calendar.
 *
 * @return {jQuery}  A reference to the calendar.
 */
snapwebsites.EditorWidgetTypeDateEdit.prototype.createCalendar = function(editor_widget, command)
{
    var that = this,
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        screen_height = parseFloat($(window.top).innerHeight()),
        scroll_top = parseFloat($(window.top).scrollTop()),
        scroll_left = parseFloat($(window.top).scrollLeft()),
        calendar = editor_widget.getData("calendar"),
        now,                // today's JavaScript Date object
        today,              // day of the month for today
        selection,          // JavaScript Date object of current selection
        new_selection,      // whether the command generated a new selection which we have to put in the widget
        no_selection,       // if true, the input is invalid so we do not have any real selection
        selection_date,     // day of the month of current selection
        selection_month,    // month of current selection (0 to 11)
        selection_year,     // full year of current selection (i.e. 2016)
        start_day_date,     // JavaScript Date of selection date with the day set to 1
        start_day,          // week day number the first day of the month falls on
        last_date_date,     // JavaScript Date of selection date with the day set to the last day of the month (i.e. +1 month -1 day)
        last_date,          // week day number the last day of the month falls on
        line,               // the day of the month while creating the calendar
        week_day,           // the calendar is one week per line, this represents the current day of the week
        day,                // JavaScript date object representing the date being output, this adjusts our 'line' parameter to a real day on the previous, current, or next month
        day_class,          // manage the class of the cell where the calendar day is output
        max_width = 0,      // maximum width from all the popup cells
        d,                  // the calendar HTML as a string
        pos,
        iframe,
        iframe_pos,
        popup_position;

    // sanity check
    if(calendar && command == "create")
    {
        throw new Error("command to the snapwebsites.EditorWidgetTypeDateEdit.prototype.createCalendar() function cannot be \"create\" if the calendar alrady exists.");
    }

    // this is today's date, we want to show today with a special border
    now = new Date();
    today = now.getDate();

    // this is the selected date, we have to show that as selected
    selection = editor_widget.getValueAsDate();
    no_selection = isNaN(selection);
    if(no_selection)
    {
        // default to today if otherwise invalid
        selection = new Date();
    }

    // move the selection as required
    new_selection = true;
    selection_date = selection.getDate();
    selection_month = selection.getMonth();
    selection_year = selection.getFullYear();
    switch(command)
    {
    case "next-year": // Ctrl-Page Up or button
        if(selection_month == 1 && selection_date == 29)
        {
            // stay in February
            selection_date = 28;
        }
        selection = new Date(selection_year + 1, selection_month, selection_date);
        break;

    case "previous-year": // Ctrl-Page Down or button
        if(selection_month == 1 && selection_date == 29)
        {
            // stay in February
            selection_date = 28;
        }
        selection = new Date(selection_year - 1, selection_month, selection_date);
        break;

    case "next-month": // Page Down or button
        selection = new Date(selection_year, selection_month + 2, 0);
        last_date = selection.getDate();
        if(selection_date > last_date)
        {
            selection_date = last_date;
        }
        selection = new Date(selection_year, selection_month + 1, selection_date);
        break;

    case "previous-month": // Page Up or button
        selection = new Date(selection_year, selection_month, 0);
        last_date = selection.getDate();
        if(selection_date > last_date)
        {
            selection_date = last_date;
        }
        selection = new Date(selection_year, selection_month - 1, selection_date);
        break;

    case "first-day-month": // Shift-Home
        selection = new Date(selection_year, selection_month, 1);
        break;

    case "last-day-month": // Shift-End
        selection = new Date(selection_year, selection_month + 1, 0);
        break;

    case "next-week": // Arrow Down
        selection = new Date(selection_year, selection_month, selection_date + 7);
        break;

    case "previous-week": // Arrow Up
        selection = new Date(selection_year, selection_month, selection_date - 7);
        break;

    case "first-day-week": // first day of the week (Home)
        week_day = selection.getDay();
        selection = new Date(selection_year, selection_month, selection_date - week_day);
        break;

    case "last-day-week": // last day of the week (End)
        week_day = selection.getDay();
        selection = new Date(selection_year, selection_month, selection_date - week_day + 6);
        break;

    case "monday": // Monday (Shift-Arrow Left)
        week_day = selection.getDay();
        selection = new Date(selection_year, selection_month, selection_date - week_day + 1);
        break;

    case "friday": // Friday (Shift-Array Right)
        week_day = selection.getDay();
        selection = new Date(selection_year, selection_month, selection_date - week_day + 5);
        break;

    case "next-day": // Arrow Right
        selection = new Date(selection_year, selection_month, selection_date + 1);
        break;

    case "previous-day": // Arrow Left
        selection = new Date(selection_year, selection_month, selection_date - 1);
        break;

    case "create":
        // no movement in this case
        new_selection = false;
        break;

    default:
        throw new Error("unknown command \"" + command + "\" for snapwebsites.EditorWidgetTypeDateEdit.prototype.createCalendar().");

    }

    if(new_selection)
    {
        no_selection = false;
    }

    // day currently selected
    selection_date = selection.getDate(); // day of the month
    selection_month = selection.getMonth();
    selection_year= selection.getFullYear();

    // first week day of this month
    start_day_date = new Date(selection_year, selection_month, 1);
    start_day = start_day_date.getDay();

    // last day of the month
    last_date_date = new Date(selection_year, selection_month + 1, 0);
    last_date = last_date_date.getDate();

    // generate the calendar, we use the widget date to determine the
    // year and month to show at first
    //
    d = "<div class='date-edit-calendar' data-year='" + selection_year
                + "' data-month='" + selection_month + "'><table><thead><tr>"
      + "<th><a href='#previous-year'>&lt;&lt;</a></th>"
      + "<th><a href='#previous-month'>&lt;</a></th>"
      + "<th colspan='3'>" + (selection_month + 1) + "/" + selection_year + "</th>"
      + "<th><a href='#next-month'>&gt;</a></th>"
      + "<th><a href='#next-year'>&gt;&gt;</a></th>"
      + "</tr><tr>";
    for(week_day = 0; week_day <= 6; ++week_day)
    {
        day = new Date(selection_year, selection_month, 1 - start_day + week_day);
        d += "<th>" + day.toLocaleDateString(navigator.language, { weekday: "narrow" }) + "</th>";
    }
    d += "</tr></thead><tbody>";
    for(line = 1 - start_day; line <= 31; )
    {
        d += "<tr>";
        for(week_day = 0; week_day <= 6; ++week_day, ++line)
        {
            day = new Date(selection_year, selection_month, line);
            day_class = "day";
            if(!no_selection
            && line == selection_date)
            {
                day_class += " selection";
            }
            if(line == today
            && selection_month == now.getMonth()
            && selection_year == now.getFullYear())
            {
                day_class += " today";
            }
            if(line < 1
            || line > last_date)
            {
                day_class += " other-month";
            }
            d += "<td class='" + day_class + "' data-day='" + line + "'><div>" + day.getDate() + "</div></td>";
        }
        d += "</tr>";
    }
    d += "</tbody></table></div>";

    // test with 'window.' so it works in IE
    pos = c.offset();
    pos.top += w.height();
    popup_position = "absolute";
    if(window.self != window.top)
    {
        iframe = jQuery(window.frameElement);

        iframe_pos = iframe.offset();
        pos.top += iframe_pos.top - scroll_top;
        pos.left += iframe_pos.left - scroll_left;

        // in the editor I have those outside the next if() block, which
        // seems strange, but I replicated so it should work the same
        //
        scroll_top = 0;
        scroll_left = 0;

        if(iframe.parents("div.snap-popup").css("position") == "fixed")
        {
            popup_position = "fixed";
        }
    }

    if(calendar)
    {
        calendar.html(d);
    }
    else
    {
        calendar = window.top.jQuery("<div class='calendar-box zordered' style='position: "
                           + popup_position + ";'>"
                           + d + "</div>").appendTo("body");
        editor_widget.setData("calendar", calendar);
    }

    // force the width of all the cells to the maximum width from any
    // of the columns
    //
    calendar.find('tr:nth-child(2) td').each(function(){
        var cell_width = parseFloat($(this).css('width'));

        if(cell_width > max_width)
        {
            max_width = cell_width;
        }
    });
    $('td').css('width', max_width);

    // verify that the calendar can fit below the widget, if not check
    // above, if neither works use the top of the screen instead
    //
    if(pos.top + calendar.height() - scroll_top > screen_height + 2) // ignore 2 lines (i.e. the +2)
    {
        // use an offset that way whether we have a fixed position or
        // an absolute position, it works
        //
        pos.top -= w.height() + calendar.height();
        if(pos.top < scroll_top)
        {
            // well... it just does not fit up either, so place it at
            // the top of the screen to increase the chance the entire
            // calendar is visible
            //
            pos.top = scroll_top;
        }
    }

    calendar.offset(pos);

    calendar
        .click(function(e)
            {
                e.preventDefault();

                snapwebsites.EditorInstance.refocus();
            })
        .mousedown(function(e)
            {
                e.preventDefault();

                that.cancelCalendarHide(editor_widget);
            })
        .find("td.day")
        .makeButton()
        .click(function(e)
            {
                var td_tag = jQuery(e.target);

                // avoid default browser behavior
                e.preventDefault();
                e.stopPropagation();

                // I cannot explain this one at this time, the click
                // may happen on the td or the div but we want to pass
                // one specific tag to the dayClicked() function
                //
                if(td_tag.prop("tagName") == "DIV")
                {
                    td_tag = td_tag.parents("td");
                }
                that.dayClicked(editor_widget, td_tag, true);
            });

    calendar
        .find("th a")
        .makeButton()
        .click(function(e)
            {
                var action = this.hash.substr(1);

                snapwebsites.EditorInstance.refocus();

                // avoid default browser behavior
                e.preventDefault();
                e.stopPropagation();

                that.createCalendar(editor_widget, action);
            });

    if(new_selection)
    {
        this.dayClicked(editor_widget, calendar.find("td[data-day='" + selection_date + "']"), false);

        // TBD: it looks like setting the widget value does not generate
        //      an input or input text event so we do not need to cancel
        //      the hide
        //
        //this.cancelCalendarHide(editor_widget);
    }

    if(command == "create")
    {
        calendar.css("display", "none");
    }

    return calendar;
};


/** \brief Hide the calendar of a Date widget.
 *
 * This function checks whether the calendar was opened. If so, then
 * it gets hidden.
 *
 * \warning
 * This function immediately hides the calendar. You may want to call
 * the startCalendarHide() instead. In many cases you do not want to
 * force the hide immediately since it may be that the system wants
 * to keep it open.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The editor widget
 *        that got focused.
 */
snapwebsites.EditorWidgetTypeDateEdit.prototype.hideCalendar = function(editor_widget)
{
    var that = this,
        calendar = editor_widget.getData("calendar");

    // make sure it was defined for this editor widget
    if(calendar)
    {
        editor_widget.setData("calendar", undefined);
        calendar.fadeOut(150, function()
            {
                calendar.remove();
            });
    }
};


/** \brief Cancel call to the startCalendarHide() function.
 *
 * This function check whether the startCalendarHide() function was called.
 * If so, then it tries to prevent it from happening.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The editor widget
 *        that got focused.
 */
snapwebsites.EditorWidgetTypeDateEdit.prototype.cancelCalendarHide = function(editor_widget)
{
    var that = this,
        timeout_id = /** @type {number} */ (editor_widget.getData("calendarTimeoutID"));

    if(!isNaN(timeout_id))
    {
        clearTimeout(timeout_id);
        editor_widget.setData("calendarTimeoutID", NaN);
    }
};


/** \brief Setup the necessary time to close the calendar popup.
 *
 * In many situations we want users to click on the calendar popup
 * and not have it closed. This function allows us to "start" the
 * hide meaning that we use a timer which allows the hide to happen
 * but only if it does not get canceled first.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The editor widget
 *        that got focused.
 */
snapwebsites.EditorWidgetTypeDateEdit.prototype.startCalendarHide = function(editor_widget)
{
    var that = this,
        timeout_id = /** @type {number} */ (editor_widget.getData("calendarTimeoutID"));

    if(isNaN(timeout_id))
    {
        timeout_id = setTimeout(function()
            {
                that.hideCalendar(editor_widget);
            },
            200);
        editor_widget.setData("calendarTimeoutID", timeout_id);
    }
};


/** \brief Handle a click on one of the calendar days.
 *
 * This function handles the click on a calendar day by using the
 * clicked date as the new selection for this DateEdit widget.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The DateEdit widget concerned.
 * @param {jQuery} day  The day that just got clicked (the td tag).
 * @param {boolean} allow_auto_hide  Allow the automatic hiding of the calendar.
 */
snapwebsites.EditorWidgetTypeDateEdit.prototype.dayClicked = function(editor_widget, day, allow_auto_hide)
{
    var w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        calendar = editor_widget.getData("calendar"),
        calendar_block = calendar.children(".date-edit-calendar"),
        year = calendar_block.data("year"),
        month = calendar_block.data("month"),
        day_of_month = day.data("day"),
        new_selection = new Date(year, month, day_of_month),
        tbody = day.parents("tbody");

    tbody.find("td").removeClass("selection");
    day.addClass("selection");

    // TODO: "older" Safari (As of 2016, NONE) do not support the
    //       toLocaleDateString() function with parameters,
    //       we should change the code in that case and generate
    //       a hard coded US date as a default fallback...
    //       https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/toLocaleDateString
    //
    editor_widget.setValue(new_selection.toLocaleDateString(navigator.language, { year: "numeric", month: "short", day: "numeric" }), true);

    // TODO: offer an option to hide the calendar once a choice was made
    //
    if(allow_auto_hide && w.hasClass("auto-hide"))
    {
        this.startCalendarHide(editor_widget);
    }

    // make sure the form is aware of the possible change
    editor_widget.getEditorBase().checkModified(editor_widget);
};



/** \brief Editor widget type for Dropdown Date widgets.
 *
 * This widget defines the "dropdown-date-edit" in the editor forms. This
 * is an equivalent to one to three dropdowns that allows one to enter a
 * date.
 *
 * @return {!snapwebsites.EditorWidgetTypeDropdownDateEdit}
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeDropdownDateEdit = function()
{
    snapwebsites.EditorWidgetTypeDropdownDateEdit.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeDropdownDateEdit, snapwebsites.EditorWidgetType);


/** \brief Return "dropdown-date-edit".
 *
 * Return the name of the widget type.
 *
 * @return {string} The name of the widget type.
 * @override
 */
snapwebsites.EditorWidgetTypeDropdownDateEdit.prototype.getType = function()
{
    return "dropdown-date-edit";
};


/** \brief Initialize the widget.
 *
 * This function initializes the date-edit widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeDropdownDateEdit.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        editor_form = editor_widget.getEditorForm(),
        name = editor_widget.getName(),
        year_widget = editor_form.getWidgetByName(name + "_year"),
        month_widget = editor_form.getWidgetByName(name + "_month"),
        day_widget = editor_form.getWidgetByName(name + "_day"),
        w;


    snapwebsites.EditorWidgetTypeDropdownDateEdit.superClass_.initializeWidget.call(this, widget);

    if(year_widget)
    {
        w = year_widget.getWidget(),
        w.bind("widgetchange", function(e)
            {
                // define the full date
                that.retrieveNewValue_(editor_widget);
            });
    }
    if(month_widget)
    {
        w = month_widget.getWidget(),
        w.bind("widgetchange", function(e)
            {
                // define the full date
                that.retrieveNewValue_(editor_widget);
            });
    }
    if(day_widget)
    {
        w = day_widget.getWidget();
        w.bind("widgetchange", function(e)
            {
                // define the full date
                that.retrieveNewValue_(editor_widget);
            });
    }
};


/** \brief Determine the new value of the timezone.
 *
 * This function gets the value of the continent and the city and
 * generates the new value of the timezone widget.
 *
 * \todo
 * Find a way to get the formchange event only if the continent or city
 * changed, and not on any change to the entire form.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The locale timezone widget.
 *
 * @private
 */
snapwebsites.EditorWidgetTypeDropdownDateEdit.prototype.retrieveNewValue_ = function(editor_widget)
{
    var editor_widget_content = editor_widget.getWidgetContent(),
        editor_form = editor_widget.getEditorForm(),
        name = editor_widget.getName(),
        month_widget = editor_form.getWidgetByName(name + "_month"),
        day_widget = editor_form.getWidgetByName(name + "_day"),
        year_widget = editor_form.getWidgetByName(name + "_year"),
        month = '-',
        day = '-',
        year = '-',
        value,
        result_value;

    if(month_widget)
    {
        value = month_widget.getValue();
        if(value)
        {
            month = value;
        }
    }
    if(day_widget)
    {
        value = day_widget.getValue();
        if(value)
        {
            day = value;
        }
    }
    if(year_widget)
    {
        value = year_widget.getValue();
        if(value)
        {
            year = value;
        }
    }

    if(month != '-' || day != '-' || year != '-')
    {
        // Force resulting date to YYYY/MM/DD
        result_value = year + '/' + month + '/' + day;
        editor_widget_content.attr("value", result_value);
    }
};



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeDateEdit());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeDropdownDateEdit());
    });

// vim: ts=4 sw=4 et
