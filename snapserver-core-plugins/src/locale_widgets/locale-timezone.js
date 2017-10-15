/** @preserve
 * Name: locale-timezone
 * Version: 0.0.1.28
 * Browsers: all
 * Depends: editor (>= 0.0.3.245)
 * Copyright: Copyright 2013-2017 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
 */

//
// Inline "command line" parameters for the Google Closure Compiler
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
 * \brief Extension to the editor: Locale Timezone Widget
 *
 * This file is used to extend the editor with a timezone widget used to
 * select a continent or country and a city or region. This is a composite
 * widget which uses two dropdown widgets to offer the user a way to define
 * their timezone.
 *
 * \code
 *      .
 *      .
 *      .
 *      |
 *  +---+--------------------+
 *  |                        |
 *  |  EditorWidgetType      |
 *  |  (cannot instantiate)  |
 *  +------------------------+
 *      ^
 *      | Inherit
 *      |
 *  +---+------------------------------+
 *  |                                  |
 *  | EditorWidgetTypeLocaleTimezone   |
 *  |                                  |
 *  +----------------------------------+
 * \endcode
 *
 * See also:
 * http://stackoverflow.com/tags/timezone/info
 */



/** \brief Editor widget type for locale timezone settings.
 *
 * This class handles the locale timezone settings form. Mainly it ensures
 * that the dropdown with the list of cities only shows cities for the
 * currently selected continent.
 *
 * \code
 *  class EditorWidgetTypeLocaleTimezone extends EditorWidgetType
 *  {
 *  public:
 *      EditorWidgetTypeLocaleTimezone() : snapwebsites.EditorWidget;
 *
 *      virtual function getType() : string;
 *      virtual function initializeWidget(widget: Object) : void;
 *
 *  private:
 *      function selectCities_(widget: snapwebsites.EditorWidget) : void;
 *  };
 * \endcode
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeLocaleTimezone = function()
{
    snapwebsites.EditorWidgetTypeLocaleTimezone.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeLocaleTimezone, snapwebsites.EditorWidgetType);


/** \brief Return "locale_timezone".
 *
 * Return the name of the locale timezone widget type.
 *
 * @return {string} The name of the locale timezone widget type.
 * @override
 */
snapwebsites.EditorWidgetTypeLocaleTimezone.prototype.getType = function() // virtual
{
    return "locale_timezone";
};


/** \brief Initialize the widget.
 *
 * This function initializes the locale timezone widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeLocaleTimezone.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        editor_form = editor_widget.getEditorForm(),
        name = editor_widget.getName(),
        continent_widget = editor_form.getWidgetByName(name + "_continent"),
        continent = continent_widget.getWidget(),
        city_widget = editor_form.getWidgetByName(name + "_city"),
        city = city_widget.getWidget();

    snapwebsites.EditorWidgetTypeLocaleTimezone.superClass_.initializeWidget.call(this, widget);

    // if continent changes, we need to update the cities
    continent.bind("widgetchange", function(e)
        {
            // fix the cities on each change
            that.selectCities_(editor_widget);
            // define the full timezone
            that.retrieveNewValue_(editor_widget);
        });

    city.bind("widgetchange", function(e)
        {
            // define the full timezone
            that.retrieveNewValue_(editor_widget);
        });

    // properly initialize the city widget (which by default shows all the
    // cities, which is "wrong")
    this.selectCities_(editor_widget);
};


/** \brief Select the cities corresponding to the current continent.
 *
 * This function hides all the cities, then shows the cities that correspond
 * to the currently selected continent. As a side effect, it auto-selects
 * the first city of that continent in case the currently selected city
 * is not otherwise valid for that continent.
 *
 * \todo
 * We want to have a way to remember the last city selected on a per
 * continent basis.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The locale timezone widget.
 *
 * @private
 */
snapwebsites.EditorWidgetTypeLocaleTimezone.prototype.selectCities_ = function(editor_widget)
{
    var editor_form = editor_widget.getEditorForm(),
        name = editor_widget.getName(),
        continent_widget = editor_form.getWidgetByName(name + "_continent"),
        //continent = continent_widget.getWidget(),
        continent_name = continent_widget.getValue(),
        city_widget = editor_form.getWidgetByName(name + "_city"),
        city = city_widget.getWidget(),
        selected;

    if(continent_name)
    {
        city_widget.enable();
        // hide all, then show corresponding to the current continent selection
        city.find("li").addClass("hidden");
        city.find("li." + continent_name).removeClass("hidden");
        selected = city.find("li.selected");
        if(!selected.exists() || !selected.hasClass(continent_name))
        {
            city_widget.setValue( /** @type {!Object} */ (city.find("li." + continent_name).first().html()), false);
        }
        // else -- user did not change continent name
    }
    else
    {
        // prevent city selection when there is no continent selected
        city_widget.disable();
        city_widget.resetValue(false);
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
snapwebsites.EditorWidgetTypeLocaleTimezone.prototype.retrieveNewValue_ = function(editor_widget)
{
    var editor_widget_content = editor_widget.getWidgetContent(),
        editor_form = editor_widget.getEditorForm(),
        name = editor_widget.getName(),
        continent_widget = editor_form.getWidgetByName(name + "_continent"),
        //continent = continent_widget.getWidget(),
        continent_name = continent_widget.getValue(),
        city_widget = editor_form.getWidgetByName(name + "_city"),
        city_name = city_widget.getValue(),
        result_value;
        //city = city_widget.getWidget();

    if(continent_name && city_name)
    {
        result_value = continent_name.replace(' ', '_') + "/" + city_name.replace(' ', '_');
        editor_widget_content.attr("value", result_value);
    }
};



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeLocaleTimezone());
    });

// vim: ts=4 sw=4 et
