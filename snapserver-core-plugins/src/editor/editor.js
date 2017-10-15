/** @preserve
 * Name: editor
 * Version: 0.0.3.949
 * Browsers: all
 * Depends: output (>= 0.1.4), popup (>= 0.1.0.1), server-access (>= 0.0.1.11), mimetype-basics (>= 0.0.3)
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



// This editor is based on the execCommand() function available in the
// JavaScript environment of browsers.
//
// This version makes use of jQuery 1.11+ to access objects. It probably is
// compatible with a little bit older versions of jQuery.
//
// Source: https://developer.mozilla.org/en-US/docs/Rich-Text_Editing_in_Mozilla
// Source: http://msdn.microsoft.com/en-us/library/ie/ms536419%28v=vs.85%29.aspx

// Other sources for the different parts:
// Source: http://stackoverflow.com/questions/10041433/how-to-detect-when-certain-div-is-out-of-view
// Source: http://stackoverflow.com/questions/5605401/insert-link-in-contenteditable-element
// Source: http://stackoverflow.com/questions/6867519/javascript-regex-to-count-whitespace-characters

// Code verification with Google Closure Compiler:
// documentation: http://code.google.com/closure/compiler/
// Source: https://github.com/google/closure-compiler
// Source: https://developers.google.com/closure/compiler/docs/error-ref
// Source: http://www.jslint.com/
//
// Command line one can use to verify the editor (jquery-for-closure.js
// may not yet be complete):
//
//   java -jar .../google-js-closure-compiler/compiler.jar \
//        --warning_level VERBOSE \
//        --js_output_file /dev/null \
//        tests/jquery-for-closure.js \
//        plugins/output/output.js \
//        plugins/editor/editor.js
//
// WARNING: output of that command is garbage as far as we're concerned
//          only the warnings are of interest.
//
// To test the compression of the script by the Google closure compiler
// (i.e. generate the .min. version) use the following parameters:
//
//   java -jar .../google-js-closure-compiler/compiler.jar \
//        --js_output_file editor.min.js
//        plugins/editor/editor.js
//
// Remember that the bare output of the closure compiler is not compatible
// with Snap!
//

/** \file
 * \brief The inline Editor of Snap!
 *
 * This file defines a set of editor features using advance JavaScript
 * classes. The resulting environment looks like this:
 *
 * \code
 * +-------------------+              +------------------+
 * |                   |              |                  |
 * |  EditorSelection  |              | EditorLinkDialog |
 * |  (all static)     |<----+        |                  |<---+
 * |                   |     |        |                  |    |
 * +-------------------+     |        +------------------+    |
 *                           |              ^                 |
 *                           | Reference    |                 |
 *                           +-----------+  | Reference       |
 *                                       |  |                 |
 * +-------------------+              +--+--+------------+    |
 * |                   |              |                  |    |
 * |   EditorBase      |  Reference   |   EditorToolbar  |    |
 * |                   |<----+--------+                  |<---+-------------------+
 * |                   |     |        |                  |                        |
 * +------------+----+-+     |        +------------------+                        |
 *   ^          |    |       |                                                    |
 *   |_Inherit  |    |       +-----------------+                                  |
 *   |          |    |                         |                                  |
 *   | Register_|    | Reference               |                                  |
 *   |    (1,n) |    |                         |                                  |
 *   |          |    v                         |                                  |
 *   |          |  +-----------------------+   |                                  |
 *   |          |  |                       |   |                                  |
 *   |          |  | EditorWidgetTypeBase  +-o | o----+                           |
 *   |          |  |                       |   |      |                           |
 *   |          |  |                       |   |      | Inherit                   |
 *   |          |  +-----------------------+   |      v                           |
 *   |          |         ^                    |   +-----------------------+      |
 *   |          |         | Inherit            |   |                       |      |
 *   |          v         |                    |   | ServerAccessCallbacks |      |
 *   |     +--------------------+              |   |                       |      |
 *   |     |                    |  Inherit     |   |                       |      |
 *   |     |  EditorWidgetType  |<--+          |   +-----------------------+      |
 *   |     |                    |   |          ^          ^                       |
 *   |     |                    |   |          |          | Inherit               |
 *   |     +--------------------+   |          |          |                       |
 *   |            ^                 |          |   +------+-----------+           |
 *   |            | Reference       |          |   |                  |           |
 *   |            |                 |          +---+  EditorFormBase  |           |
 *   |            |   +-------------+------+   |   |                  |           |
 *   |            |   |                    |   |   |                  |           ^
 *   |            |   | EditorWidgetType...|   |   +------------------+           |
 *   |            |   | (i.e. LineEdit,    |   |     ^            ^               |
 *   |            |   | Button, Checkmark) |   ^     | Inherit    | Reference     |
 *   |            |   +--------------------+   |     |            |               |
 *   |            |                            |     |     +------+-----------+   |
 *   |     +------+-------------+              |     |     |                  |   |
 *   |     |                    +--------------+     |     | EditorSaveDialog |   |
 *   |     |    EditorWidget    |                    |     |                  |   |
 *   |     |                    | Create (1,n)       |     |                  |   |
 *   |     |                    |<----------+        |     +------------------+   |
 *   |     +--------------------+           |        |           ^                |
 *   |                                      |        |           |                |
 * +-+-----------------+              +-----+--------+---+       |                |
 * |                   |              |                  +-------+                |
 * |   Editor          |              |   EditorForm     | Create (1,1)           |
 * |                   | Create (1,n) |                  |                        |
 * |                   +------------->|                  |<---------------+       |
 * +-----------------+-+              +--------+------+--+     Reference  |       |
 *       ^           |                         |      |                   |       |
 *       |           |  Create (1,1)           |      | Create (1,1)      |       |
 *       |           |                         |      v                   |       |
 *       |           |                         |   +------------------+   |       |
 *       |           |                         |   |                  |   |       |
 *       |           |                         |   |   ServerAccess   +---+       |
 *       |           |                         |   |                  |           |
 *       |           |                         |   |                  |           |
 *       |           |                         |   +------------------+           |
 *       |           |                         |                                  |
 *       |           +-------------------------+----------------->----------------+
 *       |
 *       | Create (1,1)
 *  +----+-------------------+  Create (1,n)    +------------------------+
 *  |                        +----------------->|                        |
 *  |   jQuery()             |                  | EditorWidgetType...    |
 *  |   Initialization       |                  |                        |
 *  |                        |                  |                        |
 *  +------------------------+                  +------------------------+
 * \endcode
 *
 * Note that the reference of the Toolbar from the EditorForm works
 * through the getToolbar() function of the EditorBase since the
 * toolbar object is not otherwise directly accessible. Most such
 * references work that way (i.e. by calling a function instead of
 * directly using a variable member which may still be null).
 *
 * The widget types are better defined in the following chart:
 *
 * \code
 *  +-----------------------+
 *  |                       |
 *  | ServerAccessCallbacks |
 *  | (cannot instantiate)  |
 *  +-----------------------+
 *      ^
 *      | Inherit
 *      |
 *  +------------------------+
 *  |                        |
 *  |  EditorWidgetTypeBase  |
 *  |  (cannot instantiate)  |
 *  +------------------------+
 *      ^
 *      | Inherit
 *      |
 *  +---+--------------------+
 *  |                        |  Inherit
 *  |  EditorWidgetType      |<---------------------------------------------+
 *  |  (cannot instantiate)  |                                              |
 *  +------------------------+                                              |
 *      ^                                                                   |
 *      | Inherit                                                           |
 *      |                                                                   |
 *  +---+------------------------------+    +---------------------------+   |
 *  |                                  |    |                           +---+
 *  | EditorWidgetTypeContentEditable  |    | EditorWidgetTypeCheckmark |   |
 *  | (cannot instantiate)             |    |                           |   |
 *  +----------------------------------+    +---------------------------+   |
 *      ^                                                                   |
 *      | Inherit                                                           |
 *      |                                                                   |
 *  +---+------------------------------+    +---------------------------+   |
 *  |                                  |    |                           +---+
 *  | EditorWidgetTypeTextEdit         |    | EditorWidgetTypeRadio     |   |
 *  |                                  |    |                           |   |
 *  +----------------------------------+    +---------------------------+   |
 *      ^                                                                   |
 *      | Inherit                                                           |
 *      |                                                                   |
 *  +---+------------------------------+    +---------------------------+   |
 *  |                                  |    |                           +---+
 *  | EditorWidgetTypeLineEdit         |    | EditorWidgetTypeImageBox  |   |
 *  |                                  |    |                           |   |
 *  +----------------------------------+    +---------------------------+   |
 *      ^                           ^          ^                            |
 *      | Inherit           Inherit |          | Inherit                    |
 *      |                           |          |                            |
 *  +---+----------------------+    |       +---------------------------+   |
 *  |                          |    |       |                           |   |
 *  | EditorWidgetTypeDropdown |    |       | EditorWidgetTypeDropped-  |   |
 *  |                          |    |       | FileWithPreview           |   |
 *  +--------------------------+    |       +---------------------------+   |
 *                                  |                                       |
 *  +-------------------------------+--+    +---------------------------+   |
 *  |                                  |    |                           +---+
 *  | EditorWidgetTypeDateEdit         |    | EditorWidgetTypeDropped-  |   |
 *  |                                  |    | File                      |   |
 *  +----------------------------------+    +---------------------------+   |
 *                                                                          |
 *                                          +---------------------------+   |
 *                                          |                           +---+
 *                                          | EditorWidgetTypeSilent    |   |
 *                                          |                           |   |
 *                                          +---------------------------+   |
 *                                                                          |
 *                                          +---------------------------+   |
 *                                          |                           +---+
 *                                          | EditorWidgetTypeHidden    |
 *                                          |                           |
 *                                          +---------------------------+
 * \endcode
 *
 * An interesting talk about what a good editor should be considered to be:
 *
 * https://medium.com/medium-eng/why-contenteditable-is-terrible-122d8a40e480#.qep2uqp7k
 */


/** \brief Snap EditorSelection constructor.
 *
 * The EditorSelection is a set of functions used to deal with the selection
 * property of the different browsers (each browser has a different API at
 * the moment...)
 *
 * The functions are pretty much all considered static functions as they
 * do not need any data. Therefore this is not an object, just a set of
 * functions.
 *
 * @struct
 */
snapwebsites.EditorSelection =
{
    /** \brief Trim the selection.
     *
     * This function checks whether the selection starts and ends with
     * spaces. If so, those spaces are removed from the selection. This
     * makes for much cleaner links.
     *
     * The function takes no parameter since there can be only one current
     * selection and the system (document) knows about it.
     */
    trimSelectionText: function() // static
    {
        var range, sel, text, trimStart, trimEnd;

        if(document.selection)
        {
            range = document.selection.createRange();
            text = range.text;
            trimStart = text.match(/^\s*/)[0].length;
            if(trimStart)
            {
                range.moveStart("character", trimStart);
            }
            trimEnd = text.match(/\s*$/)[0].length;
            if(trimEnd)
            {
                range.moveEnd("character", -trimEnd);
            }
            range.select();
        }
        else
        {
            sel = window.getSelection();
            if(sel.getRangeAt)
            {
                range = sel.getRangeAt(0);
                text = range.toString();
                trimStart = text.match(/^\s*/)[0].length;
                if(trimStart && range.startContainer)
                {
                    range.setStart(range.startContainer, range.startOffset + trimStart);
                }
                trimEnd = text.match(/\s*$/)[0].length;
                if(trimEnd && range.endContainer)
                {
                    range.setEnd(range.endContainer, range.endOffset - trimEnd);
                }
            }
        }
    },

    /** \brief Retrieve the selection boundary element.
     *
     * This function determines the tag that encompasses the current
     * selection.
     *
     * This is particularly useful when editing a link and making sure that
     * the entire link is selected before you edit anything.
     *
     * \note
     * This function does not modify the selection.
     *
     * @param {boolean} isStart  Whether the start end end container should be used.
     *
     * @return {Element}  The object representing the selection boundaries.
     */
    getSelectionBoundaryElement: function(isStart) // static
    {
        var range, sel, container;

        if(document.selection)
        {
            // Note that IE offers a RangeText here, not a Range
            range = document.selection.createRange();
            if(range)
            {
                range.collapse(isStart);
                return range.parentElement();
            }
        }
        else
        {
            sel = window.getSelection();
            if(sel.getRangeAt)
            {
                if(sel.rangeCount > 0)
                {
                    range = sel.getRangeAt(0);
                }
            }
            else
            {
                // Old WebKit
                range = document.createRange();
                range.setStart(sel.anchorNode, sel.anchorOffset);
                range.setEnd(sel.focusNode, sel.focusOffset);

                // Handle the case when the selection was selected backwards (from the end to the start in the document)
                if(range.collapsed !== sel.isCollapsed)
                {
                    range.setStart(sel.focusNode, sel.focusOffset);
                    range.setEnd(sel.anchorNode, sel.anchorOffset);
                }
            }

            if(range)
            {
               container = range[isStart ? "startContainer" : "endContainer"];

               // Check if the container is a text node and return its parent if so
               return container.nodeType === 3 ? container.parentNode : container;
            }
        }

        return null;
    },

    /** \brief Set the boundary to the element.
     *
     * This function changes the selection so it matches the start and end
     * point of the specified element (tag).
     *
     * @param {Element} tag  The tag to which the selection is adjusted.
     */
    setSelectionBoundaryElement: function(tag) // static
    {
        // This works since IE9
        var range = document.createRange(),
            sel;

        range.setStartBefore(tag);
        range.setEndAfter(tag);
        sel = window.getSelection();
        sel.removeAllRanges();
        sel.addRange(range);
    },

    /** \brief Retrieve the current selection.
     *
     * This function retrieves the current selection and returns it. The
     * caller is responsible to save the selection information and restore
     * it later with a call to restoreSelection().
     *
     * In general this function is used to save the selection before doing
     * an action that will mess with the current selection (i.e. open a
     * dialog which has widgets and thus a new selection.)
     *
     * @return {Object}  An object representing the current selection.
     */
    saveSelection: function() // static
    {
        var sel;

        if(document.selection)
        {
            return document.selection.createRange();
        }
        else
        {
            sel = window.getSelection();
            if(sel.getRangeAt && sel.rangeCount > 0)
            {
                return sel.getRangeAt(0);
            }
            else
            {
                return null;
            }
        }
        //NOTREACHED
    },

    /** \brief Restore a selection previously saved with the saveSelection().
     *
     * This function expects a selection as a parameter as returned by the
     * saveSelection() function.
     *
     * The caller is responsible for only restoring selections that make
     * sense. If the selection represents something that already disappeared,
     * then it should not be applied.
     *
     * \todo
     * Ameliorate the input type using the possible Range types.
     *
     * @param {Object} range  The range describing the selection to restore.
     */
    restoreSelection: function(range) // static
    {
        var sel;

        if(document.selection)
        {
            range.select();
        }
        else
        {
            sel = window.getSelection();
            sel.removeAllRanges();
            sel.addRange(/** @type {Range} */ (range));
        }
    },

    /** \brief Search for links in the selection.
     *
     * This function searches the selection for links and returns an array
     * of all the links. It may return an empty array.
     *
     * @return {Array.<string>}  The list of links found in the current selection.
     */
    getLinksInSelection: function() // static
    {
        var i, r, sel, selectedLinks = [],
            range, containerEl, links, linkRange;

        if(window.getSelection)
        {
            sel = window.getSelection();
            if(sel.getRangeAt && sel.rangeCount)
            {
                linkRange = document.createRange();
                for(r = 0; r < sel.rangeCount; ++r)
                {
                    range = sel.getRangeAt(r);
                    containerEl = range.commonAncestorContainer;
                    if(containerEl.nodeType != 1)
                    {
                        containerEl = containerEl.parentNode;
                    }
                    if(containerEl.nodeName.toLowerCase() === "a")
                    {
                        selectedLinks.push(containerEl);
                    }
                    else
                    {
                        links = containerEl.getElementsByTagName("a");
                        for(i = 0; i < links.length; ++i)
                        {
                            linkRange.selectNodeContents(links[i]);
                            if(linkRange.compareBoundaryPoints(range.END_TO_START, range) < 1
                            && linkRange.compareBoundaryPoints(range.START_TO_END, range) > -1)
                            {
                                selectedLinks.push(links[i]);
                            }
                        }
                    }
                }
                linkRange.detach();
            }
        }
        else if(document.selection && document.selection.type !== "Control")
        {
            range = document.selection.createRange();
            containerEl = range.parentElement();
            if(containerEl.nodeName.toLowerCase() === "a")
            {
                selectedLinks.push(containerEl);
            }
            else
            {
                links = containerEl.getElementsByTagName("a");
                linkRange = document.body.createTextRange();
                for(i = 0; i < links.length; ++i)
                {
                    linkRange.moveToElementText(links[i]);
                    if(linkRange.compareEndPoints("StartToEnd", range) > -1 && linkRange.compareEndPoints("EndToStart", range) < 1)
                    {
                        selectedLinks.push(links[i]);
                    }
                }
            }
        }
        return selectedLinks;
    },

    /** \brief Get the text currently selected.
     *
     * This function gets the text currently selected.
     *
     * \note
     * This function does not verify that the text that is currently
     * selected corresponds to the element currently marked as the
     * current widget in the editor. However, if everything goes
     * according to plan, it will always be the case.
     *
     * \todo
     * Ameliorate the return type using the proper Range definitions.
     *
     * @return {string}  The text currently selected.
     */
    getSelectionText: function() // static
    {
        var range, sel;

        if(document.selection)
        {
            range = document.selection.createRange();
            return range.text;
        }

        sel = window.getSelection();
        if(sel.getRangeAt)
        {
            range = sel.getRangeAt(0);
            return range.toString();
        }

        return '';
    }

//pasteHtmlAtCaret: function(html, selectPastedContent)
//{
//    var sel, range;
//    if(window.getSelection)
//    {
//        // IE9 and non-IE
//        sel = window.getSelection();
//        if(sel.getRangeAt && sel.rangeCount)
//        {
//            range = sel.getRangeAt(0);
//            range.deleteContents();
//
//            // Range.createContextualFragment() would be useful here but is
//            // only relatively recently standardized and is not supported in
//            // some browsers (IE9, for one)
//            var el = document.createElement("div");
//            el.innerHTML = html;
//            var frag = document.createDocumentFragment(), node, lastNode;
//            while((node = el.firstChild))
//            {
//                lastNode = frag.appendChild(node);
//            }
//            var firstNode = frag.firstChild;
//            range.insertNode(frag);
//
//            // Preserve the selection
//            if(lastNode)
//            {
//                range = range.cloneRange();
//                range.setStartAfter(lastNode);
//                if(selectPastedContent)
//                {
//                    range.setStartBefore(firstNode);
//                }
//                else
//                {
//                    range.collapse(true);
//                }
//                sel.removeAllRanges();
//                sel.addRange(range);
//            }
//        }
//    }
//    else if((sel = document.selection) && sel.type != "Control")
//    {
//        // IE < 9
//        var originalRange = sel.createRange();
//        originalRange.collapse(true);
//        sel.createRange().pasteHTML(html);
//        if(selectPastedContent)
//        {
//            range = sel.createRange();
//            range.setEndPoint("StartToStart", originalRange);
//            range.select();
//        }
//    }
//},

//    findFirstVisibleTextNode_: function(p)
//    {
//        var textNodes, nonWhitespaceMatcher = /\S/;
//
//        function findFirstVisibleTextNode__(p)
//        {
//            var i, max, result, child;
//
//            // text node?
//            if(p.nodeType == 3) // Node.TEXT_NODE == 3
//            {
//                return p;
//            }
//
//            max = p.childNodes.length;
//            for(i = 0; i < max; ++i)
//            {
//                child = p.childNodes[i];
//
//                // verify visibility first (avoid walking the tree of hidden nodes)
////console.log("node " + child + " has display = [" + jQuery(child).css("display") + "]");
////                if(jQuery(child).css("display") == "none")
////                {
////                    return result;
////                }
//
//                result = findFirstVisibleTextNode__(child);
//                if(result)
//                {
//                    return result;
//                }
//            }
//        }
//
//        return findFirstVisibleTextNode__(p);
//    },

//    resetSelectionText: function()
//    {
//        var range, selection;
//        if(document.selection)
//        {
//            range = document.selection.createRange();
//            range.moveStart("character", 0);
//            range.moveEnd("character", 0);
//            range.select();
//            return;
//        }
//
//console.log("active is ["+this.activeElement_+"]");
//        var elem=this.findFirstVisibleTextNode_(this.activeElement_);//jQuery(this.activeElement_).filter(":visible:text:first");
//console.log("first elem = ["+this.activeElement_+"/"+jQuery(elem).length+"] asis:["+elem+"] text:["+elem.nodeValue+"] node:["+jQuery(elem).prop("nodeType")+"]");
////jQuery(this.activeElement_).find("*").filter(":visible").each(function(i,e){
////console.log(" + child = ["+this+"] ["+jQuery(this).prop("tagName")+"] node:["+jQuery(this).prop("nodeType")+"]");
////});
//
//        range = document.createRange();//Create a range (a range is a like the selection but invisible)
//        range.selectNodeContents(elem);//this.activeElement_);//Select the entire contents of the element with the range
//        //range.collapse(true);//collapse the range to the end point. false means collapse to end rather than the start
//        selection = window.getSelection();//get the selection object (allows you to change selection)
//        selection.removeAllRanges();//remove any selections already made
//        selection.addRange(range);//make the range you have just created the visible selection
//        return;
//
//
//        var sel = window.getSelection();
//        if(sel.getRangeAt)
//        {
//            range = sel.getRangeAt(0);
//console.log('Current position: ['+range.startOffset+'] inside ['+jQuery(range.startContainer).attr("class")+']');
//            range.setStart(this.activeElement_, 0);
//            range.setEnd(this.activeElement_, 0);
//        }
//    },

};



/** \brief Snap EditorWidgetTypeBase constructor.
 *
 * The editor works with widgets that are based on this class. Each widget
 * is given a type such as "line-edit".
 *
 * To make it fully dynamic, we define a base class here and let other
 * programmers add new widget types in their own .js files by extending
 * this class.
 *
 * Classes must be registered with the EditorBase class function:
 *
 * \code
 *    snapwebsites.EditorInstance.registerWidgetType(your_widget_type);
 * \endcode
 *
 * This base class already implements a few things that are common to
 * all widgets.
 *
 * \code
 *  class EditorWidgetTypeBase extends ServerAccessCallbacks
 *  {
 *  public:
 *      EditorWidgetTypeBase();
 *
 *      // structure of data getting saved
 *      class SaveData
 *      {
 *          html: string;
 *          result: string;
 *      };
 *
 *      abstract function getType() : string;
 *      abstract function preInitializeWidget(widget: Object) : Void;
 *      abstract function initializeWidget(editor_widget: Object) : Void;
 *      virtual function saving(editor_widget: Object, data: EditorWidgetTypeBase.SaveData) : Void;
 *      virtual function resetValue(editor_widget: Object) : Void;
 *      virtual function setValue(editor_widget: Object, value: Object) : Void;
 *      virtual function validate(editor_widget: Object) : boolean;
 *
 *  private:
 *      var ajaxReason_: string = "";
 *      var ajaxWidget_: EditorWidget = "";
 *      var serverAccess_: ServerAccess = null;
 *  };
 * \endcode
 *
 * @constructor
 * @extends snapwebsites.ServerAccessCallbacks
 * @struct
 */
snapwebsites.EditorWidgetTypeBase = function()
{
    // TBD
    // Maybe at some point we will want to create yet another layer
    // so we can have an auto-register, but I am not totally sure
    // that would really work right in all cases...
    //snapwebsites.EditorBase.registerWidgetType(this);

    return this;
};


/** \brief EditorWidgetTypeBase inherits from the ServerAccessCallbacks class.
 *
 * This class inherits from the snapwebsites.ServerAccessCallbacks so that
 * way widgets can send AJAX data to the server and handle the response.
 *
 * Note that if you inherit from a widget and implements these functions,
 * make sure to call the super version too in case they do something that
 * is required. At this point the base class does nothing in those callbacks
 * although we may add error handling in the error callback.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeBase, snapwebsites.ServerAccessCallbacks);


/** \brief The type of parameter one can pass to the save functions.
 *
 * The result is saved in a data object which fields are:
 *
 * \li html -- the data being saved, used to change the originalData_
 *               field once successfully saved.
 * \li result -- the data to send to the server.
 *
 * @typedef {{html: string, result: string}}
 */
snapwebsites.EditorWidgetTypeBase.SaveData;


/** \brief Defines what the serverAccess_ object is used for.
 *
 * The ajaxReason_ is a string that represents this widget doing
 * with the current AJAX request. This is used whenever we receive
 * the serverAccessSuccess(), serverAccessError(), and
 * serverAccessComplete() functions.
 *
 * By default, it is set to the empty string, meaning that no AJAX
 * is going on.
 *
 * @type {string}
 * @private
 */
snapwebsites.EditorWidgetTypeBase.prototype.ajaxReason_ = "";


/** \brief Hold a reference to the widget in link with the last AJAX request.
 *
 * Whenever the editor needs to send an AJAX request, it saves the
 * reason in ajaxReason_ and the concerned width in ajaxWidget_.
 *
 * @type {snapwebsites.EditorWidget}
 * @private
 */
snapwebsites.EditorWidgetTypeBase.prototype.ajaxWidget_ = null;


/** \brief A server access object.
 *
 * This object is used differently depending on the widget and user
 * actions.
 *
 * For example, EditorWidgetTypeDroppedFileWithPreview makes use of this
 * server to check on the validity, type, etc. of the dropped file.
 *
 * \note
 * This serverAccess_ object is shared between all the drop widget.
 * For this reason, we have another field, ajaxReason_, which tells
 * us what the serverAccess_ is.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.EditorWidgetTypeBase.prototype.serverAccess_ = null;


/** \brief Retrieve the name of this widget type.
 *
 * This function returns the widget type. It is used when you
 * register the type in the snap editor object. It may be called at
 * any other time to be able to distinguish between types that are derived
 * from each others.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (required override.)
 *
 * @return {string}  The type of this editor widget type as a string.
 */
snapwebsites.EditorWidgetTypeBase.prototype.getType = function() // virtual
{
    throw new Error("snapwebsites.EditorWidgetTypeBase.getType() was not overloaded.");
};


/*jslint unparam: true */
/** \brief Pre-initialize a widget of this type.
 *
 * The parameter is really a snapwebsites.EditorWidget object,
 * but at this point we did not yet define that object type.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override.)
 *
 * @param {!Object} editor_widget  The widget being initialized.
 */
snapwebsites.EditorWidgetTypeBase.prototype.preInitializeWidget = function(editor_widget) // virtual
{
    throw new Error("snapwebsites.EditorWidgetTypeBase.preInitializeWidget() does not do anything (yet)");
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Initialize a widget of this type.
 *
 * The parameter is really a snapwebsites.EditorWidget object,
 * but at this point we did not yet define that object type.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override.)
 *
 * @param {!Object} editor_widget  The widget being initialized.
 */
snapwebsites.EditorWidgetTypeBase.prototype.initializeWidget = function(editor_widget) // virtual
{
    throw new Error("snapwebsites.EditorWidgetTypeBase.initializeWidget() doesn't do anything (yet)");
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Allow for special handling of the widget data when saving.
 *
 * This function is called whenever the data of a widget is to be
 * sent to the server.
 *
 * @param {!Object} editor_widget  The concerned widget
 * @param {!Object.<snapwebsites.EditorWidgetTypeBase.SaveData>} data  The data to be saved.
 */
snapwebsites.EditorWidgetTypeBase.prototype.saving = function(editor_widget, data) // virtual
{
    // by default we do nothing (the defaults created by the
    // snapwebsites.EditorWidget.saving() function are good enough.)
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Reset the value of a widget.
 *
 * This function offers a way for programmers to dynamically reset the
 * value of a widget.
 *
 * At this point it is specifically used to reset a Dropdown. A Dropdown
 * widget may have a "reset value" which is not one of the values one
 * can select in the list of items of the Dropdown.
 *
 * @param {!Object} editor_widget  The concerned widget
 *
 * @return {boolean}  true if the value got set.
 */
snapwebsites.EditorWidgetTypeBase.prototype.resetValue = function(editor_widget) // virtual
{
    // by default we do nothing here, which means nothing gets reset by
    // default... a type must overload this function so something
    // happens

    return false;
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Save a new value in the specified editor widget.
 *
 * This function offers a way for programmers to dynamically change the
 * value of a widget. You should never call the editor widget type
 * function, instead use the setValue() function of the widget you
 * want to change the value of (it will make sure that the modified
 * flag is properly set.)
 *
 * Depending on the type, the value may be a string, a number, of some
 * other type, this is why here it is marked as an object.
 *
 * @param {!Object} editor_widget  The concerned widget
 * @param {!Object|string|number} value  The value to be saved.
 *
 * @return {boolean}  true if the value got set.
 */
snapwebsites.EditorWidgetTypeBase.prototype.setValue = function(editor_widget, value) // virtual
{
    // by default we do nothing here, which means nothing gets set by
    // default... a type must overload this function so something
    // happens

    return false;
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Validate the content of a widget.
 *
 * This function is expected to check the value of a widget to make
 * sure it is valid. For example, it will make sure it is not too
 * short nor too long.
 *
 * By default, the function returns true so it validates any data.
 *
 * @param {Object} editor_widget  The widget to validate.
 *
 * @return {boolean}  true if the widget content is currently considered valid.
 */
snapwebsites.EditorWidgetTypeBase.prototype.validate = function(editor_widget) // virtual
{
    return true;
};
/*jslint unparam: false */



/** \brief Snap EditorBase constructor.
 *
 * The base editor includes everything that the toolbar and the form objects
 * need to reference in the editor although some of those parameters are
 * created by the main editor object (i.e. the toolbar and forms are created
 * by the main editor object.)
 *
 * \code
 * class EditorBase
 * {
 * public:
 *      virtual function getToolbar() : EditorToolbar;
 *      function setActiveElement(element: jQuery);
 *      function getActiveElement() : jQuery;
 *      function refocus();
 *      virtual function checkModified(editor_widget);
 *      virtual function getLinkDialog() : EditorLinkDialog;
 *      virtual function registerWidgetType(widget_type: EditorWidgetType);
 *      function hasWidgetType(type_name: string) : boolean;
 *      function getWidgetType(type_name: string) : EditorWidgetType;
 * private:
 *      activeElement_: Element;
 *      widgetTypes_: EditorWidgetType;
 * };
 * \endcode
 *
 * @return {snapwebsites.EditorBase}  The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.EditorBase = function()
{
//#ifdef DEBUG
    if(jQuery("body").hasClass("snap-editor-initialized"))
    {
        throw new Error("Only one editor singleton can be created.");
    }
    jQuery("body").addClass("snap-editor-initialized");
//#endif

    this.widgetTypes_ = [];

    return this;
};


/** \brief Mark EditorBase as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.EditorBase);


/** \brief The height of your fixed header.
 *
 * When a theme has a fixed header, fields can end up hidden under that
 * header. When the height of that fixed header is specified using this
 * function (see setFixedHeaderHeight(),) then the editor can adjust the
 * scroll position to make sure that the top of the field appears in view.
 *
 * Actually, you can even add the height of the label of that field to
 * also show that label if it appears above the field.
 *
 * @type {number}
 * @private
 */
snapwebsites.EditorBase.prototype.fixedHeaderHeight_ = 0;


/** \brief The currently active element.
 *
 * The element considered active in the editor is the very element
 * that gets the focus.
 *
 * When no elements are focused, it is expected to be null. However,
 * our current code may not always properly reset it.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorBase.prototype.activeElement_ = null;


/** \brief The list of widget types understood by the editor.
 *
 * This array is used to save the widget types when you call the
 * registerWidgetType() function.
 *
 * @type {Array.<snapwebsites.EditorWidgetType>}
 * @private
 */
snapwebsites.EditorBase.prototype.widgetTypes_; // = []; -- initialized in constructor to avoid problems


/** \brief Retrieve the toolbar object.
 *
 * This function returns a reference to the toolbar object.
 *
 * \exception
 * This function raises an error exception if called directly (i.e. the
 * funtion is virtual).
 *
 * @return {snapwebsites.EditorToolbar}  The toolbar object.
 */
snapwebsites.EditorBase.prototype.getToolbar = function() // virtual
{
    throw new Error("getToolbar() cannot directly be called on the EditorBase class.");
};


/** \brief Set the fixed header height.
 *
 * If you have a theme with a fixed header, fields are going to be hidden
 * by the header whether they gain focus or not. Setting this value to the
 * height of your fixed header will adjust the position of the window
 * scroll so the focused fields will appear in view.
 *
 * The height may include the fixed header height plus the height of the
 * label if it appears on the previous line. That way both, the field and
 * its label will appear on the screen.
 *
 * Setting the height to zero or a negative number cancels this feature.
 *
 * @param {number} new_height  The height of the fixed header.
 */
snapwebsites.EditorBase.prototype.setFixedHeaderHeight = function(new_height)
{
    this.fixedHeaderHeight_ = new_height;
};


/** \brief Retrieve the fixed header height.
 *
 * When a widget is given focus, it may be under the fixed header at the top
 * of your screen. When that happens, the editor will scroll the widget
 * into view.
 *
 * However, for that to happen, the editor needs to know the height of your
 * headers. It is defined by the setFixedHeaderHeight() function and later
 * retrieved using this getFixedHeaderHeight() function.
 *
 * Note that the height may include the height of your labels if you'd like
 * to see your label appear in the screen.
 *
 * @return {number} The fixed header height.
 */
snapwebsites.EditorBase.prototype.getFixedHeaderHeight = function()
{
    return this.fixedHeaderHeight_;
};


/** \brief Define the active element.
 *
 * Whenever an editor object receives the focus, this function is called
 * with that widget. The element is expected to be a valid jQuery object.
 *
 * Note that the blur() event cannot directly set the active element to
 * \em null because at that point the toolbar may have been clicked and
 * in that case we don't want to really lose the focus...
 *
 * @param {jQuery} element  A jQuery element representing the focused object.
 *
 * @final
 */
snapwebsites.EditorBase.prototype.setActiveElement = function(element)
{
//#ifdef DEBUG
    if(element !== null)
    {
        if(!(element instanceof jQuery))
        {
            throw new Error("setActiveElement() must be called with a jQuery object.");
        }
        if(!element.is(".editor-content"))
        {
            throw new Error("setActiveElement() must be called with a jQuery object of a editor-content object. The class attribute of this object is \"" + element.attr("class") + "\".");
        }
    }
//#endif
    this.activeElement_ = element;
};


/** \brief Retrieve the currently active element.
 *
 * This function returns a reference to the currently focused element.
 *
 * @return {jQuery} The DOM element with the focus.
 *
 * @final
 */
snapwebsites.EditorBase.prototype.getActiveElement = function()
{
    //jQuery(snapwebsites.EditorInstance.activeElement_)
    return this.activeElement_;
};


/** \brief Refocus the active element.
 *
 * This function is used by the toolbar to refocus the currently
 * active element because when one clicks on the toolbar, the focus
 * is lost from the active element.
 *
 * @final
 */
snapwebsites.EditorBase.prototype.refocus = function()
{
    if(this.activeElement_)
    {
        this.activeElement_.focus();
    }
};


/** \brief Check whether a field was modified.
 *
 * When something may have changed (a character may have been inserted
 * or deleted, or a text replaced) then you are expected to call this
 * function in order to see whether something was indeed modified.
 *
 * When the process detects that something was modified, it calls the
 * necessary functions to open the Save Dialog.
 *
 * As a side effect it also lets the toolbar know so if it needs to be
 * moved, it happens.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget that
 *                               generates the checkModified() call.
 *
 * @throws {Error}  The base class throws.
 */
snapwebsites.EditorBase.prototype.checkModified = function(editor_widget) // virtual
{
    throw new Error("checkModified() cannot directly be called on the EditorBase class.");
};


/** \brief Retrieve the link dialog.
 *
 * This function creates an instance of the link dialog and returns it.
 * If the function gets called more than once, then the same reference
 * is returned.
 *
 * @return {snapwebsites.EditorLinkDialog}  The link dialog reference.
 *
 * @throws {Error}  The base class implementation just throws.
 */
snapwebsites.EditorBase.prototype.getLinkDialog = function() // virtual
{
    throw new Error("getLinkDialog() cannot directly be called on the EditorBase class.");
};


/** \brief Register a widget type.
 *
 * This function is used to register a widget type in the editor.
 * This allows for any number of extensions and thus any number of
 * cool advanced features that do not all need to be defined in the
 * core of the editor.
 *
 * @param {snapwebsites.EditorWidgetType} widget_type  The widget type to register.
 */
snapwebsites.EditorBase.prototype.registerWidgetType = function(widget_type) // virtual
{
    var name = widget_type.getType();
    this.widgetTypes_[name] = widget_type;
};


/** \brief Check whether a type exists.
 *
 * This function checks the list of widget types to see whether
 * \p type_name exists.
 *
 * @param {!string} type_name  The name of the type to check.
 *
 * @return {!boolean}  true if the type is defined.
 *
 * @final
 */
snapwebsites.EditorBase.prototype.hasWidgetType = function(type_name)
{
    return this.widgetTypes_[type_name] instanceof snapwebsites.EditorWidgetTypeBase;
};


/** \brief This function is used to get a widget type.
 *
 * Widget types get registered with the registerWidgetType() function.
 * Later you may retrieve them using this function.
 *
 * @throws {Error} If the named \p type was not yet registered, then this
 *                 function throws.
 *
 * @param {string} type_name  The name of the widget type to retrieve.
 *
 * @return {snapwebsites.EditorWidgetType} The widget type object.
 *
 * @final
 */
snapwebsites.EditorBase.prototype.getWidgetType = function(type_name)
{
    if(!(this.widgetTypes_[type_name] instanceof snapwebsites.EditorWidgetTypeBase))
    {
        throw new Error("getWidgetType() of type \"" + type_name + "\" is not yet defined, you cannot get it now.");
    }
    return this.widgetTypes_[type_name];
};



/** \brief Snap EditorLinkDialog constructor.
 *
 * The editor link dialog is a popup window that is used to let the user
 * enter a link (URI, anchor, and some other parameters of the anchor.)
 *
 * @param {snapwebsites.EditorBase} editor_base  A reference to the editor
 *                                               base object.
 *
 * @return {snapwebsites.EditorLinkDialog} A reference to the object being
 *                                         initialized.
 * @constructor
 * @struct
 */
snapwebsites.EditorLinkDialog = function(editor_base)
{
    var that = this;

    this.editorBase_ = editor_base;

    // TODO: add support for a close without saving the changes!
    var html = "<div id='snap_editor_link_dialog'>"
             + "<div class='title'>Link Administration</div>"
             + "<div id='snap_editor_link_page'>"
             + "<div class='line'><label class='limited' for='snap_editor_link_text'>Text:</label> <input id='snap_editor_link_text' name='text' title='Enter the text representing the link. If none, the link will appear as itself.'/></div>"
             + "<div class='line'><label class='limited' for='snap_editor_link_url'>Link:</label> <input id='snap_editor_link_url' name='url' title='Enter a URL.'/></div>"
             + "<div class='line'><label class='limited' for='snap_editor_link_title'>Tooltip:</label> <input id='snap_editor_link_title' name='title' title='The tooltip which appears when a user hovers the mouse cursor over the link.'/></div>"
             + "<div class='line'><label class='limited'>&nbsp;</label><input id='snap_editor_link_new_window' type='checkbox' value='' title='Click to save your changes.'/> <label for='snap_editor_link_new_window'>New Window</label></div>"
             + "<div class='line'><label class='limited'>&nbsp;</label><input id='snap_editor_link_ok' type='button' value='OK' title='Click to save your changes.'/></div>"
             + "<div style='clear:both;padding:0;'></div></div></div>";

    jQuery(html).appendTo("body");
    this.linkDialogPopup_ = jQuery("#snap_editor_link_dialog");

    jQuery("#snap_editor_link_ok").click(function(e)
        {
            e.preventDefault();
            that.close();
        });

    return this;
};


/** \brief Mark EditorLinkDialog as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.EditorLinkDialog);


/** \brief The editor base to access the active widget.
 *
 * A reference to the EditorBase object. This allows us to access the
 * currently active widget and apply different functions that are
 * useful to get the editor to work (actually, all the commands... so
 * that's a great deal of the editor!)
 *
 * This parameter is always initialized as it is set when the
 * EditorLinkDialog is constructed.
 *
 * @type {snapwebsites.EditorBase}
 * @private
 */
snapwebsites.EditorLinkDialog.prototype.editorBase_ = null;


/** \brief The jQuery Link Dialog object.
 *
 * This variable holds the jQuery object representing the Link Dialog
 * DOM object. The object is created at the time the EditorLinkDialog
 * is created so as far as this object is concerned, it is pretty much
 * always available.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorLinkDialog.prototype.linkDialogPopup_ = null;


/** \brief A DarkenScreen used with the link dialog.
 *
 * This references the DarkenScreen used with the link popup dialog
 * which we manually manage in the editor.
 *
 * @type {snapwebsites.DarkenScreen}
 * @private
 */
snapwebsites.EditorLinkDialog.prototype.linkDialogDarkenScreen_ = null;


/** \brief The selection range when opening a dialog.
 *
 * The selection needs to be preserved whenever we open a popup dialog
 * in the editor. This selection is saved in this variable. The
 * selection itself is gathered using the saveSelection() and later
 * restored with the restoreSelection() functions.
 *
 * @type {Object}
 * @private
 */
snapwebsites.EditorLinkDialog.prototype.selectionRange_ = null;


/** \brief Open the link dialog.
 *
 * This function opens (i.e. shows and positions) the link dialog.
 *
 * A strong side effect of this function is to darken anything
 * else in the background.
 */
snapwebsites.EditorLinkDialog.prototype.open = function()
{
    this.selectionRange_ = snapwebsites.EditorSelection.saveSelection();

    var jtag,
        selectionText = snapwebsites.EditorSelection.getSelectionText(),
        links = snapwebsites.EditorSelection.getLinksInSelection(),
        new_window = true,
        focusItem,
        pos,
        height,
        left,
        title,
        z;

    if(links.length > 0)
    {
        jtag = jQuery(links[0]);
        // it is already the anchor, we can use the text here
        // in this case we also have a URL and possibly a title
        jQuery("#snap_editor_link_url").val(snapwebsites.castToString(jtag.attr("href"), "href in #snap_editor_link_url"));

        // the title is not mandatory so we may get "undefined"
        title = jtag.attr("title");
        if(typeof title == "string")
        {
            jQuery("#snap_editor_link_title").val(snapwebsites.castToString(title, "title in #snap_editor_link_title"));
        }

        // the target is also optional
        new_window = jtag.attr("target") === "_blank";
    }
    else
    {
        jtag = jQuery(snapwebsites.EditorSelection.getSelectionBoundaryElement(true));

        // this is not yet the anchor, we need to retrieve the selection
        //
        // TODO detect email addresses
        if(selectionText.substr(0, 7) === "http://"
        || selectionText.substr(0, 8) === "https://"
        || selectionText.substr(0, 6) === "ftp://")
        {
            // selection is a URL so make use of it
            jQuery("#snap_editor_link_url").val(selectionText);
        }
        else
        {
            jQuery("#snap_editor_link_url").val("");
        }
        jQuery("#snap_editor_link_title").val("");
    }
    jQuery("#snap_editor_link_text").val(selectionText);
    jQuery("#snap_editor_link_new_window").prop('checked', new_window);
    if(selectionText.length)
    {
        focusItem = "#snap_editor_link_url";
    }
    else
    {
        focusItem = "#snap_editor_link_text";
    }
    pos = jtag.position();
    height = jtag.outerHeight(true);
    this.linkDialogPopup_.css("top", pos.top + height);
    left = pos.left - 5;
    if(left < 10)
    {
        left = 10;
    }
    this.linkDialogDarkenScreen_ = new snapwebsites.DarkenScreen(150, false);
    this.linkDialogPopup_.css("left", left);
    this.linkDialogPopup_.css("z-index", 0);
    z = jQuery(".zordered").maxZIndex() + 1;
    this.linkDialogPopup_.css("z-index", z);
    this.linkDialogPopup_.fadeIn(300, function()
        {
            jQuery(focusItem).focus();
        });
};


/** \brief Close the editor link dialog.
 *
 * This function copies the link to the widget being edited and
 * closes the popup.
 */
snapwebsites.EditorLinkDialog.prototype.close = function()
{
    var url, links, jtag, text, title, new_window;

    this.linkDialogPopup_.fadeOut(150);
    this.linkDialogDarkenScreen_.close();
    this.linkDialogDarkenScreen_ = null;

    this.editorBase_.refocus();
    snapwebsites.EditorSelection.restoreSelection(this.selectionRange_);
    url = jQuery("#snap_editor_link_url");
    document.execCommand("createLink", false, url.val());
    links = snapwebsites.EditorSelection.getLinksInSelection();
    if(links.length > 0)
    {
        jtag = jQuery(links[0]);
        text = jQuery("#snap_editor_link_text");
        if(text.length > 0)
        {
            jtag.text(snapwebsites.castToString(text.val(), "val() of #snap_editor_link_text"));
        }
        // do NOT erase the existing text if the user OKed
        // without any text

        title = jQuery("#snap_editor_link_title");
        if(title.length > 0)
        {
            jtag.attr("title", snapwebsites.castToString(title.val(), "val() of #snap_editor_link_title"));
        }
        else
        {
            jtag.removeAttr("title");
        }

        new_window = jQuery("#snap_editor_link_new_window");
        if(new_window.prop("checked"))
        {
            jtag.attr("target", "_blank");
        }
        else
        {
            jtag.removeAttr("target");
        }
    }
};



/** \brief Snap EditorToolbar constructor.
 *
 * Whenever creating the Snap! Editor a toolbar comes with it (although
 * it can be hidden, the Ctrl-T key can be used to show/hide the bar.)
 *
 * The toolbar is created as a separate object and a reference is saved
 * in each EditorForm. The bar can only be opened once for the widget
 * with the focus.
 *
 * @param {snapwebsites.EditorBase} editor_base  A reference to the editor base object.
 *
 * @return {snapwebsites.EditorToolbar} The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.EditorToolbar = function(editor_base)
{
    // save the reference to the editor base
    this.editorBase_ = editor_base;
    this.keys_ = [];

    return this;
};


/** \brief Mark EditorToolbar as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.EditorToolbar);


/** \brief the list of buttons in the Toolbar.
 *
 * This array defines the list of buttons available in the toolbar.
 * The JavaScript code makes use of this information to generate the
 * toolbar and later to handle key presses that the editor understands.
 *
 * \note
 * This array should be a constant but actually we want to fix a few
 * things here and there depending on the browser and whatever other
 * feature. Also at some point we should offer a way for other
 * plugins to add their own buttons.
 *
 * @type {Array.<Array.<(Object|string|number|function(Array))>>}
 * @const
 * @private
 */
snapwebsites.EditorToolbar.toolbarButtons_ = // static const
[
    // TODO: support for translations, still need to determine how we
    //       want to do that, at this point I think it should be
    //       separate .js files and depending on the language, the user
    //       includes the right file... (i.e. editor-1.2.3-en.js)
    //
    // TODO: support a way for other plugins to add (remove?) functions
    //       in a clean way, at this point inserting in this array would
    //       not be clean at all...

    //
    // WARNING: Some control keys cannot be used under different browsers
    //          (especially Internet Explorer which does not care much
    //          whether you try to capture those controls.)
    //
    //    . Ctrl-O -- open a new URL
    //

    // Structure:
    //
    //    command -- the exact command you use with
    //               document.execDocument(), if "|" it is a group
    //               separator; if "*" it is an internal command
    //               (not compatible with document.execDocument().)
    //
    //    title -- the title of the command, this is displayed as
    //             buttons are hovered with the mouse pointer; this
    //             entry can be null if no title is available and in
    //             that case the button is not added to the DOM toolbar
    //
    //    key & flags -- this field includes the key that activates
    //                   the command; (i.e. "Bold (Ctrl-B)"); the key
    //                   is expected to be 16 bits; the higher bits are
    //                   used as flags:
    //
    //                   0x0001:0000 -- requires the shift key
    //                   0x0002:0000 -- fix the selection so it better
    //                                  matches a link selection
    //                   0x0004:0000 -- run a dynamic callback such as
    //                                  the '_linkDialog'
    //
    //    parameter -- a parameter for the command, it may be for the
    //                 execDocument() or dynamic callback as defined
    //                 by flag 0x0004:0000.
    //
    //    parameter -- another parameter for the command, for example
    //                 the name of dynamic callback when flag
    //                 0x0004:0000 is set
    //

    ["bold", "Bold (Ctrl-B)", 0x42],
    ["italic", "Italic (Ctrl-I)", 0x49],
    ["underline", "Underline (Ctrl-U)", 0x55],
    ["strikeThrough", "Strike Through (Ctrl-Shift--)", 0x100AD],
    ["removeFormat", "Remove Format (Ctlr-Shift-Delete)", 0x1002E],
    ["|", "|"],
    ["subscript", "Subscript (Ctrl-Shift-B)", 0x10042],
    ["superscript", "Superscript (Ctrl-Shift-P)", 0x10050],
    ["|", "|"],
    ["createLink", "Manage Link (Ctrl-L)", 0x6004C, "http://snapwebsites.org/" /*, snapwebsites.EditorToolbar.prototype.callbackLinkDialog_*/],
    ["unlink", "Remove Link (Ctrl-K)", 0x2004B],
    ["|", "-"],
    ["insertUnorderedList", "Bulleted List (Ctrl-Q)", 0x51],
    ["insertOrderedList", "Numbered List (Ctrl-Shift-O)", 0x1004F],
    ["|", "|"],
    ["outdent", "Decrease Indent (Ctrl-Up)", 0x26],
    ["indent", "Increase Indent (Ctrl-Down)", 0x28],
    ["formatBlock", "Block Quote (Ctrl-Shift-Q)", 0x10051, "<blockquote>"],
    ["insertHorizontalRule", "Horizontal Line (Ctrl-H)", 0x48],
    ["insertFieldset", "Fieldset (Ctrl-Shift-S)", 0x10053],
    ["|", "|"],
    ["justifyLeft", "Left Align (Ctrl-Shift-L)", 0x1004C],
    ["justifyCenter", "Centered (Ctrl-E)", 0x45],
    ["justifyRight", "Right Align (Ctrl-Shift-R)", 0x10052],
    ["justifyFull", "Justified (Ctrl-J)", 0x4A],

    // no buttons,  just the keyboard at this point
    ["formatBlock", null, 0x31, "<h1>"],        // Ctrl-1
    ["formatBlock", null, 0x32, "<h2>"],        // Ctrl-2
    ["formatBlock", null, 0x33, "<h3>"],        // Ctrl-3
    ["formatBlock", null, 0x34, "<h4>"],        // Ctrl-4
    ["formatBlock", null, 0x35, "<h5>"],        // Ctrl-5
    ["formatBlock", null, 0x36, "<h6>"],        // Ctrl-6
    ["fontSize", null, 0x10031, "1"],           // Ctrl-Shift-1
    ["fontSize", null, 0x10032, "2"],           // Ctrl-Shift-2
    ["fontSize", null, 0x10033, "3"],           // Ctrl-Shift-3
    ["fontSize", null, 0x10034, "4"],           // Ctrl-Shift-4
    ["fontSize", null, 0x10035, "5"],           // Ctrl-Shift-5
    ["fontSize", null, 0x10036, "6"],           // Ctrl-Shift-6
    ["fontSize", null, 0x10037, "7"],           // Ctrl-Shift-7
    ["fontName", null, 0x10046, "Arial"],       // Ctrl-Shift-F -- TODO add font selector
    ["foreColor", null, 0x52, "red"],           // Ctrl-R -- TODO add color selector
    ["hiliteColor", null, 0x10048, "#ffff00"],  // Ctrl-Shift-H -- TODO add color selector
    ["*", null, 0x54, "" /*, snapwebsites.EditorToolbar.prototype.callbackToggleToolbar_*/]            // Ctrl-T
];


/** \brief The editor base to access the active widget.
 *
 * A reference to the EditorBase object. This allows us to access the
 * currently active widget and apply different functions that are
 * useful to get the editor to work (actually, all the commands... so
 * that's a great deal of the editor!)
 *
 * This parameter is always initialized as it is set when the
 * EditorToolbar is constructed.
 *
 * @type {snapwebsites.EditorBase}
 * @private
 */
snapwebsites.EditorToolbar.prototype.editorBase_ = null;


/** \brief The list of keys understood by the toolbar.
 *
 * Each toolbar button (and non-buttons) can have a key assigned to it.
 * The keys_ variable is a map of those keys to very quickly find the
 * command that corresponds to a key. This table is generated at the
 * time the toolbar is initialized.
 *
 * @type {Array.<number>}
 * @private
 */
snapwebsites.EditorToolbar.prototype.keys_; // = []; -- initialized in the constructor to avoid problems


/** \brief The jQuery toolbar
 *
 * This is the jQuery object representing the toolbar (a \<div\>
 * tag in the DOM.)
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorToolbar.prototype.toolbar_ = null;


/** \brief Whether the toolbar is currently visible.
 *
 * The toolbar can be shown with:
 *
 * \code
 * toggleToolbar(true);
 * \endcode
 *
 * The toolbar can be hidden with:
 *
 * \code
 * toggleToolbar(false);
 * \endcode
 *
 * The toolbar can be toggled from visible to not visible with:
 *
 * \code
 * toggleToolbar();
 * \endcode
 *
 * This flag represents the current state.
 *
 * Note, however, that the toolbar may be fading in or out. In
 * that case this flag already reflects the final state.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorToolbar.prototype.toolbarVisible_ = false;


/** \brief Whether the toolbar is shown at the bottom of the widget.
 *
 * When there isn't enough room to put the toolbar at the top, this
 * flag is set to true. The default is rather meaningful.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorToolbar.prototype.bottomToolbar_ = false;


/** \brief Current height of the widget the toolbar is open for.
 *
 * This value represents the height of the widget currently having
 * the focus. The height is saved as it is a lot faster to access
 * it in this way than retrieving it each time. It also gives us
 * the ability to detect that the height changed and adjust the
 * position of the toolbar if necessary.
 *
 * @type {number}
 * @private
 */
snapwebsites.EditorToolbar.prototype.height_ = -1;


/** \brief The identifier of the last timer created.
 *
 * Whenever the current widget loses focus, a timer is used to
 * eventually hides the toolbar. The problem is that when someone
 * clicks on the toolbar, the focus gets lost, so just closing
 * the toolbar at that point is not a good idea.
 *
 * @type {number}
 *
 * @private
 */
snapwebsites.EditorToolbar.prototype.toolbarTimeoutID_ = NaN;


/** \brief Call this function whenever the toolbar is about to be accessed.
 *
 * This function is called whenever the EditorToolbar object is about to
 * access the toolbar DOM. This allows the system to create the toolbar
 * only once required. This is whenever an editor key (i.e. Ctrl-B) is
 * hit or if the toolbar is to be shown.
 *
 * @private
 */
snapwebsites.EditorToolbar.prototype.createToolbar_ = function()
{
    var that = this, msie, html, originalName, isGroup, idx, max;

    if(!this.toolbar_)
    {
        msie = /msie/.exec(navigator.userAgent.toLowerCase()); // IE?
        html = "<div id=\"toolbar\">";
        max = snapwebsites.EditorToolbar.toolbarButtons_.length;

        for(idx = 0; idx < max; ++idx)
        {
            // the name of the image always uses the original name
            originalName = snapwebsites.EditorToolbar.toolbarButtons_[idx][0];

            if(originalName == "createLink")
            {
                snapwebsites.EditorToolbar.toolbarButtons_[idx][4] = snapwebsites.EditorToolbar.prototype.callbackLinkDialog_;
            }
            else if(originalName == "*")
            {
                snapwebsites.EditorToolbar.toolbarButtons_[idx][4] = snapwebsites.EditorToolbar.prototype.callbackToggleToolbar_;
            }

            if(msie)
            {
                if(snapwebsites.EditorToolbar.toolbarButtons_[idx][0] === "hiliteColor")
                {
                    snapwebsites.EditorToolbar.toolbarButtons_[idx][0] = "backColor";
                }
            }
            else
            {
                if(snapwebsites.EditorToolbar.toolbarButtons_[idx][0] === "insertFieldset")
                {
                    snapwebsites.EditorToolbar.toolbarButtons_[idx][0] = "insertHTML";
                    snapwebsites.EditorToolbar.toolbarButtons_[idx][3] = "<fieldset><legend>Fieldset</legend><p>&nbsp;</p></fieldset>";
                }
            }
            isGroup = snapwebsites.EditorToolbar.toolbarButtons_[idx][0] === "|";
            if(!isGroup)
            {
                this.keys_[snapwebsites.EditorToolbar.toolbarButtons_[idx][2] & 0x1FFFF] = idx;
            }
            if(snapwebsites.EditorToolbar.toolbarButtons_[idx][1] !== null)
            {
                if(isGroup)
                {
                    if(snapwebsites.EditorToolbar.toolbarButtons_[idx][1] === "-")
                    {
                        // horizontal separator, create a new line
                        html += "<div class=\"horizontal-separator\"></div>";
                    }
                    else
                    {
                        // vertical separator, show a small vertical bar
                        html += "<div class=\"group\"></div>";
                    }
                }
                else
                {
                    // Buttons
                    html += "<div unselectable=\"on\" class=\"button "
                            + originalName
                            + "\" button-id=\"" + idx + "\" title=\""
                            + snapwebsites.EditorToolbar.toolbarButtons_[idx][1] + "\">"
                            + "<span class=\"image\"></span></div>";
                }
            }
        }
        html += "</div>";
        jQuery(html).appendTo("body");
        this.toolbar_ = jQuery("#toolbar");

        this.toolbar_
            .click(function(e)
                {
                    e.preventDefault();

                    that.editorBase_.refocus();
                })
            .mousedown(function(e)
                {
                    e.preventDefault();

                    // XXX: this needs to be handled through a form of callback
                    that.cancelToolbarHide();
                })
            .find(":any(.horizontal-separator .group)")
                .click(function()
                    {
                        that.editorBase_.refocus();
                    });

        this.toolbar_
            .find(".button")
                .click(function()
                    {
                        var index = parseInt(jQuery(this).attr("button-id"), 10);
                        that.editorBase_.refocus();
                        that.command(index);
                    });
    }
};


/** \brief Execute a toolbar command.
 *
 * This function expects the index of the command to execute. The
 * command index is the index of the command in the toolbarButtons_
 * array.
 *
 * @throws {Error} In debug version, raise this error if something invalid
 *                 is detected at runtime.
 *
 * @param {number} idx  The index of the command to execute.
 *
 * @return {boolean}  Return true to indicate that the command was
 *                    processed.
 */
snapwebsites.EditorToolbar.prototype.command = function(idx)
{
    var tag, callback;

    // command is defined?
    if(!snapwebsites.EditorToolbar.toolbarButtons_[idx])
    {
        return false;
    }

//console.log("run command "+idx+" "+snapwebsites.EditorToolbar.toolbarButtons_[idx][2]+"!!!");

    // require better selection for a link? (i.e. full link)
    if(snapwebsites.EditorToolbar.toolbarButtons_[idx][2] & 0x20000)
    {
        snapwebsites.EditorSelection.trimSelectionText();
        tag = snapwebsites.EditorSelection.getSelectionBoundaryElement(true);
        if(jQuery(tag).prop("tagName") === "A")
        {
            // if we get here the whole tag was not selected,
            // select it now
            snapwebsites.EditorSelection.setSelectionBoundaryElement(tag);
        }
    }

    if(snapwebsites.EditorToolbar.toolbarButtons_[idx][0] === "*"
    || snapwebsites.EditorToolbar.toolbarButtons_[idx][2] & 0x40000)
    {
        // "internal command" which does not return immediately
        callback = snapwebsites.EditorToolbar.toolbarButtons_[idx][4];
//#ifdef DEBUG
        if(typeof callback !== "function")
        {
            throw new Error("snapwebsites.Editor.command() callback #" + idx + " function \"" + callback + "\" is not a function.");
        }
//#endif
        callback.apply(this, snapwebsites.EditorToolbar.toolbarButtons_[idx]);

        if(snapwebsites.EditorToolbar.toolbarButtons_[idx][0] === "*")
        {
            // the dialog OK button will do the rest of the work as
            // required by this specific command
            return true;
        }
    }
    else
    {
        // TODO: We probably want to transform all of those calls
        //       as callbacks just so it is uniform and clean...
        //       (even if the callbacks end up using execCommand()
        //       pretty much as is.)
        //
        // if there is a toolbar parameter, make sure to pass it along
        if(snapwebsites.EditorToolbar.toolbarButtons_[idx][3])
        {
            // TODO: need to define the toolbar parameter
            //       (i.e. a color, font name, size, etc.)
            document.execCommand(snapwebsites.castToString(snapwebsites.EditorToolbar.toolbarButtons_[idx][0], "toolbar command with parameter"),
                                    false, snapwebsites.EditorToolbar.toolbarButtons_[idx][3]);
        }
        else
        {
            document.execCommand(snapwebsites.castToString(snapwebsites.EditorToolbar.toolbarButtons_[idx][0], "toolbar command, no parameters"),
                                    false, null);
        }
    }
    this.editorBase_.checkModified(null);

    return true;
};


/** \brief Process a keydown event.
 *
 * This function checks whether the key that was just pressed represents
 * a command, if so execute it.
 *
 * The function returns true if the key was used by the toolbar, which
 * means it should not be used by anything else afterward.
 *
 * \note
 * The function only makes use of keys if the Control key was pushed
 * down.
 *
 * @param {Event} e  The jQuery event object.
 *
 * @return {boolean}  If true, the key was used up by the toolbar process.
 */
snapwebsites.EditorToolbar.prototype.keydown = function(e)
{
    if(e.ctrlKey)
    {
        return this.command(this.keys_[e.which + (e.shiftKey ? 0x10000 : 0)]);
    }
    return false;
};


/** \brief Check whether the toolbar should be moved.
 *
 * As the user enters data in the focused widget, the toolbar may need to
 * be adjusted.
 *
 * There are several possibilities as defined here:
 *
 * 1. The toolbar fits above the widget and is visible, place it there;
 *
 * 2. The toolbar doesn't fit above, place it at the bottom;
 *
 * 3. If the widget grows, make sure that the toolbar remains at the
 *    bottom if it doesn't fit at the top;
 *
 * 4. As the page is scrolled up and down, make sure that the toolbar
 *    remains visible as required for proper editing.
 *
 * \todo
 * Currently our positioning is extremely limited and does not work in
 * all cases. We'll have to do updates once we have the time to fix
 * all the problems. Also, we want to look into creating one function
 * to calculate the position, and then this and the toggleToolbar()
 * functions can make sure to place th object at the right location.
 * Also we may want to offer the user ways to select the toolbar
 * location (and shape) so on long pages it could be at the top, the
 * bottom or a side.
 */
snapwebsites.EditorToolbar.prototype.checkPosition = function()
{
    var newHeight, pos;

    if(snapwebsites.EditorInstance.bottomToolbar_)
    {
        newHeight = snapwebsites.castToNumber(jQuery(this).outerHeight(), "casting outer height to a number");
        if(newHeight != snapwebsites.EditorInstance.height_)
        {
            snapwebsites.EditorInstance.height_ = newHeight;
            pos = jQuery(this).position();
            snapwebsites.EditorInstance.toolbar_.animate({top: pos.top + jQuery(this).height() + 3}, 200);
        }
    }
};


/** \brief Show, hide, or toogle the toolbar visibility.
 *
 * This function is used to show (true), hide (false), or toggle
 * (undefined, do not specify a parameter) the visibility of the
 * toolbar.
 *
 * The function is responsible for placing the toolbar at the right
 * place so it is visible and does not obstruct the field being
 * edited.
 *
 * \todo
 * Ameliorate the positioning of the toolbar, and especially, make sure
 * it remains visible even when editing very tall or wide widgets (i.e.
 * the body of a page can span many \em visible pages.) See the
 * checkPosition() function and the fact that we want ONE common function
 * to calculate the position requirements.
 *
 * @param {boolean=} force  Whether to show (true), hide (false),
 *                         or toggle (undefined) the visibility.
 */
snapwebsites.EditorToolbar.prototype.toggleToolbar = function(force)
{
    var toolbarHeight, snap_editor_element, pos, widget;

    if(force === true || force === false)
    {
        this.toolbarVisible_ = force;
    }
    else
    {
        this.toolbarVisible_ = !this.toolbarVisible_;
    }

    if(this.toolbarVisible_)
    {
        // make sure to cancel the fade out in case it is active
        this.cancelToolbarHide();

        // in this case we definitvely need to have a toolbar, create it
        this.createToolbar_();

        // start showing the bar, then place it (when display is none,
        // the size is incorrect, calling fadeIn() first fixes that
        // problem.)
        this.toolbar_.fadeIn(300);
        widget = this.editorBase_.getActiveElement();
        snap_editor_element = widget.parents('.snap-editor');
        this.height_ = snapwebsites.castToNumber(snap_editor_element.height(), ".snap-editor height");
        toolbarHeight = this.toolbar_.outerHeight();
        pos = snap_editor_element.position();
        this.bottomToolbar_ = pos.top < toolbarHeight + 5;
        if(this.bottomToolbar_)
        {
            // too low, put the toolbar at the bottom
            this.toolbar_.css("top", (pos.top + this.height_ + 3) + "px");
        }
        else
        {
            this.toolbar_.css("top", (pos.top - toolbarHeight - 3) + "px");
        }
        this.toolbar_.css("left", (pos.left + 5) + "px");
    }
    else if(this.toolbar_) // if no toolbar anyway, exit
    {
        // since we're hiding it now, cancel the toolbar timer
        this.cancelToolbarHide();

        // make sure it is gone
        this.toolbar_.fadeOut(150);
    }
};


/*jslint unparam: true */
/** \brief ToggleToobar callback to support the Ctrl-T key.
 *
 * This function is called when the user hits the Ctrl-T key. It toggles
 * the visibility of the toolbar.
 *
 * @param {string} cmd  The command that generated this call.
 * @param {?string} title  The command that generated this call.
 * @param {number} key_n_flags  The command that generated this call.
 * @param {string|number|null=} opt_param  The command that generated this call.
 * @param {function(this: snapwebsites.EditorToolbar, string, ?string, number, (string|number|null), function())=} opt_func  The command that generated this call.
 *
 * @private
 */
snapwebsites.EditorToolbar.prototype.callbackToggleToolbar_ = function(cmd, title, key_n_flags, opt_param, opt_func)
{
    this.toggleToolbar();
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Callback to define a link.
 *
 * This function is the callback that opens a popup to edit a link in
 * the editor. It makes use of the EditorLinkDialog.
 *
 * @param {string} cmd  The command that generated this call.
 * @param {?string} title  The command that generated this call.
 * @param {number} key_n_flags  The command that generated this call.
 * @param {string|number|null=} opt_param  The command that generated this call.
 * @param {function(this: snapwebsites.EditorToolbar, string, ?string, number, (string|number|null), function())=} opt_func  The command that generated this call.
 *
 * @private
 */
snapwebsites.EditorToolbar.prototype.callbackLinkDialog_ = function(cmd, title, key_n_flags, opt_param, opt_func)
{
    // for now we can ignore cmd

    this.editorBase_.getLinkDialog().open();
};
/*jslint unparam: false */


/** \brief Cancel the toolbar hide timer.
 *
 * When a widget loses focus we start a timer to be used to hide
 * the toolbar. This widget may get the focus again before the
 * toolbar gets completely hidden. This function can be called
 * to cancel that timer.
 */
snapwebsites.EditorToolbar.prototype.cancelToolbarHide = function()
{
    if(!isNaN(this.toolbarTimeoutID_))
    {
        // prevent hiding of the toolbar
        clearTimeout(this.toolbarTimeoutID_);
        this.toolbarTimeoutID_ = NaN;
    }
};


/** \brief Start the toolbar hide feature.
 *
 * This function starts a timer which, if not cancelled in time, will
 * close the toolbar.
 *
 * It is used whenever a widget loses focus.
 */
snapwebsites.EditorToolbar.prototype.startToolbarHide = function()
{
    var that = this;

    if(isNaN(this.toolbarTimeoutID_))
    {
        this.toolbarTimeoutID_ = setTimeout(function()
            {
                // although the toggleToolbar() function is likely to
                // call the cancelToolbarHide() ["likely" because there
                // is one case where it could be that it does not happen],
                // it is not clean to have it clear a timer that was just
                // triggered...
                //
                that.toolbarTimeoutID_ = NaN;

                that.toggleToolbar(false);
            },
            200);
    }
};



/** \brief Snap EditorWidget constructor.
 *
 * For each of the widget you add to your editor forms, one of these
 * is created on the client system.
 *
 * \code
 * class EditorWidget
 * {
 * public:
 *      function EditorWidget(editor_base: EditorBase, editor_form: EditorForm, widget: jQuery);
 *      final function getName() : string;
 *      final function getLabel(use_name: boolean) : string;
 *      final function wasModified(opt_recheck: boolean) : boolean;
 *      final function saving() : SaveData;
 *      final function saved(data: SaveData) : boolean;
 *      final function discard();
 *      function getEditorForm() : EditorForm;
 *      function getEditorBase() : EditorBase;
 *      function focus() : Void;
 *      function blur() : Void;
 *      function show() : Void;
 *      function hide() : Void;
 *      function enable(opt_state: boolean) : Void;
 *      function disable() : Void;
 *      function isEnable() : boolean;
 *      final function getWidget() : jQuery;
 *      final function getWidgetContent() : jQuery;
 *      final function checkForBackgroundValue();
 *      static final function isEmptyBlock(html: string|jQuery) : boolean;
 *      function getValue() : string;
 *      function getValueAsDate() : Date;
 *      function restoreValue() : boolean;
 *      function resetValue(changed: boolean) : Void;
 *      function setValue(value: Object, changed: boolean) : Void;
 *      function getWidgetType() : EditorWidgetTypeBase;
 *      function showWaitImage() : Void;
 *      function hideWaitImage() : Void;
 *      function getData(name: string) : Object;
 *      function setData(name: string, data: Object) : Void;
 *
 * private:
 *      function rotateWaitImage_() : Void;
 *
 *      var editorBase_: EditorBase = null;
 *      var editorForm_: EditorForm = null;
 *      var widget_: jQuery = null;
 *      var widgetContent_: jQuery = null;
 *      var name_: string;
 *      var originalData_: string = "";
 *      var originalValue_: string = undefined;
 *      var modified_: boolean = false;
 *      var widgetType_: EditorWidgetTypeBase = null;
 * };
 * \endcode
 *
 * \todo
 * It looks like we are using EditorForm before it gets defined.
 *
 * @param {snapwebsites.EditorBase} editor_base  A reference to the editor base object.
 * @param {snapwebsites.EditorForm} editor_form  A reference to the editor form object that owns this widget.
 * @param {jQuery} widget  The jQuery object representing this editor toolbar.
 *
 * @return {snapwebsites.EditorWidget} The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.EditorWidget = function(editor_base, editor_form, widget)
{
    var type = snapwebsites.castToString(widget.attr("field_type"), "field_type attribute"),
        that = this;

    this.editorBase_ = editor_base;
    this.editorForm_ = editor_form;
    this.widget_ = widget; // this is the jQuery widget (.snap-editor)
    this.name_ = snapwebsites.castToString(widget.attr("field_name"), "field_name attribute");
    this.widgetContent_ = widget.children(".editor-content");
    if(this.widgetContent_.length == 0)
    {
        throw new Error("Widget \"" + this.name_ + "\" must define a child element with class \"editor-content\"");
    }
    // Moved to AFTER the [pre]initialization
    //this.originalData_ = snapwebsites.castToString(this.widgetContent_.html(), "widgetContent HTML in EditorWidget constructor for " + this.name_);
    this.widgetType_ = editor_base.getWidgetType(type);
    this.widgetData_ = [];
    this.checkForBackgroundValue();

//#ifdef DEBUG
    // these should be caught on the server side too but this test
    // ensures that we catch invalid names of dynamically created widgets
    // (i.e. in most cases the widget names are "hard coded" in your XSLT
    // files and as such can be checked even before we add the file to the
    // layout table)
    if(this.name_.length > 0
    && this.name_[0] === "_")
    {
        throw new Error("The widget name \"" + this.name_ + "\" cannot start with an underscore as such names are reserved by the system.");
    }
//#endif

    // this should be last
    this.widgetType_.preInitializeWidget(this);

    return this;
};


/** \brief Mark EditorWidget as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.EditorWidget);


/** \brief The editor base object.
 *
 * The a reference back to the editor base object. This is pretty much
 * the same as the snapwebsites.EditorInstance reference.
 *
 * @type {snapwebsites.EditorBase}
 * @private
 */
snapwebsites.EditorWidget.prototype.editorBase_ = null;


/** \brief The form that owns this widget.
 *
 * This a reference back to the editor form object.
 *
 * @type {snapwebsites.EditorForm}
 * @private
 */
snapwebsites.EditorWidget.prototype.editorForm_ = null;


/** \brief The jQuery widget.
 *
 * This parameter represents the jQuery widget. The actual visible
 * editor widget.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorWidget.prototype.widget_ = null;


/** \brief A jQuery wait widget.
 *
 * This parameter represents a jQuery widget created when the
 * showWaitImage() is called.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorWidget.prototype.waitWidget_ = null;


/** \brief The timer identifier.
 *
 * This parameter is the identifier returned by the setInterval() function.
 *
 * @type {number}
 * @private
 */
snapwebsites.EditorWidget.prototype.waitWidgetTimerID_ = NaN;


/** \brief A jQuery wait widget current rotation position.
 *
 * This parameter represents the position of the image in the wait
 * widget. The wait widget has a background which position is moved
 * from 0 to -704 so it looks as it was rotating.
 *
 * @type {number}
 * @private
 */
snapwebsites.EditorWidget.prototype.waitWidgetPosition_ = 0;


/** \brief The jQuery widget representing the content of the widget.
 *
 * This parameter is the DIV under the widget which is marked as the
 * "editor-content" (i.e. with the class named "editor-content").
 * This is the part that we manage in the different widget
 * implementations.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorWidget.prototype.widgetContent_ = null;


/** \brief The name of the widget.
 *
 * Whenever the widget is created this field is set to the name
 * of the widget (the "field_name" attribute.)
 *
 * @const
 * @type {!string}
 * @private
 */
snapwebsites.EditorWidget.prototype.name_;


/** \brief The original data of the widget.
 *
 * Until saved, this data is taken from the existing widget at
 * initialization time.
 *
 * @type {string}
 * @private
 */
snapwebsites.EditorWidget.prototype.originalData_ = "";


/** \brief The original value of the widget.
 *
 * Some widgets use a value="..." parameter to save their value.
 * For example, the dropdown widget shows a label to the user
 * and uses a value for its result.
 *
 * By default, the original value is undefined.
 *
 * This is particularly important when you call the restoreValue()
 * function which has to remove a value if present when the original
 * was undefined.
 *
 * @type {!string}
 * @private
 */
snapwebsites.EditorWidget.prototype.originalValue_;


/** \brief Whether the system detected that the widget was modified.
 *
 * To avoid comparing possibly very large buffers (originalData_ and
 * the widget html() data) we save the last result in this variable.
 * Once it is true, we just return true when calling the wasModified()
 * function.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorWidget.prototype.modified_ = false;


/** \brief The type of this widget.
 *
 * The object representing the type of this widget. It is used to
 * finish the initialization of the widget (i.e. connect to the
 * signals, etc.)
 *
 * @type {snapwebsites.EditorWidgetTypeBase}
 * @private
 */
snapwebsites.EditorWidget.prototype.widgetType_ = null;


/** \brief User defined data.
 *
 * A map of objects that, in most cases, the widget type handles.
 *
 * @type {Object}
 * @private
 */
snapwebsites.EditorWidget.prototype.widgetData_ = null; // = []; -- initialized in constructor to avoid inter-object sharing problems


/** \brief Get the name of this widget.
 *
 * On creation the widget retrieves the attribute named "field_name"
 * which contains the name of the widget (used to communicate
 * with the server.) This function returns that name.
 *
 * @return {string}  The name of the widget.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.getName = function()
{
    return this.name_;
};


/** \brief Retrieve the label of this widget.
 *
 * The label of a widget is expected to be defined in a \<label> tag
 * with the \em for attribute set to the name of the widget. If such
 * is found, then the function returns the HTML text defined within
 * that label (the HTML is kept as is.)
 *
 * If no such label exists, the function returns an empty string unless
 * you set the \p use_name parameter to true, in that case we return
 * the \em technical widget name (as if you had called the getName()
 * function.)
 *
 * @param {boolean} use_name  Whether to return the widget name if no
 *                            label is found.
 *
 * @return {string}  The label or the name of the widget.
 */
snapwebsites.EditorWidget.prototype.getLabel = function(use_name)
{
    // TODO: look into a way to avoid searching the entire DOM
    //       (i.e. in most cases it will be a sibling of this.widget_)
    //
    var label = jQuery("label[for='" + this.name_ + "']");

    // got a valid label?
    if(label.exists())
    {
        // if there is a span with an asterisk (or whatnot) to mark
        // the field as "required", remove it, it is not really
        // useful within the label
        //
        label = label.clone();
        jQuery("span.required", label).remove();
        label = /** @type {string} */ (label.html());
        label = snapwebsites.trim(label.replace(/[\s]*:[\s]*$/, ""));
        return snapwebsites.trim(snapwebsites.castToString(label, "expected html() to return a string for the widget label"));
    }

    return use_name ? this.name_ : "";
};


/** \brief Check whether the widget was modified.
 *
 * This function compares the widget old and current data to see
 * whether it changed.
 *
 * @param {boolean=} opt_recheck  Whether we want to force a check of
 *                                the current HTML data with the original.
 *
 * @return {boolean}  Whether the widget was modified (true) or not
 *                    (false).
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.wasModified = function(opt_recheck)
{
    var value;

    if(opt_recheck || !this.modified_)
    {
        value = this.widgetContent_.attr("value");
        if(this.originalValue_ !== undefined
        && value !== undefined)
        {
            // if there is an originalValue_ then we have to check
            // against the value="..." attribute and not the HTML
            //
            this.modified_ = snapwebsites.trim(this.originalValue_) != snapwebsites.trim(snapwebsites.castToString(value, "expected attr() for \"" + this.name_ + "\" to return a string in this case"));
        }
        else
        {
//#ifdef DEBUG
            if(value === undefined && this.widget_.hasClass("composite"))
            {
                throw new Error("Composite widget \"" + this.name_ + "\" must define a \"value\" attribute to work properly.");
            }
//#endif
            this.modified_ = snapwebsites.trim(this.originalData_) != snapwebsites.trim(snapwebsites.castToString(this.widgetContent_.html(), "expected html() for \"" + this.name_ + "\" to return a string in this case"));
        }
    }

    return this.modified_;
};


/** \brief Validate the widget content.
 *
 * This function gets called to validate the widget whenever the user
 * is ready to save the form.
 *
 * In general the validation consists of making sure that the content
 * is valid as per the programmer's specification. For example, a
 * line-edit needs to have a minimum and maximum number of characters.
 * The maximum is checked as new characters are added, but the minimum
 * is only checked before the save and thus in this validation function.
 *
 * @return {boolean}  true if the widget is considered valid.
 */
snapwebsites.EditorWidget.prototype.validate = function()
{
    return this.widgetType_.validate(this);
};


/** \brief Get the data to be saved.
 *
 * This function is called whenever the widget is marked as modified
 * or the form as "always save all". It retrieves the data being
 * saved and the result to be sent to the server. The result may
 * be quite optimized, for example, in case of radio buttons, just
 * one (small) value is sent to the server.
 *
 * By default the data.result gets cleaned up so no starting or
 * ending blanks are kept. It also removes \<br\> tags that have
 * no attributes.
 *
 * This function cannot be overridden. There is no need because the
 * editor creates the EditorWidget objects anyway. Instead you want
 * to derive the saving() function of the widget type.
 *
 * @return {snapwebsites.EditorWidgetTypeBase.SaveData}  The data to be saved.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.saving = function()
{
    var value,          // in case the widget defines a value attribute
        data = {};      // the data to be returned

    data.html = snapwebsites.castToString(this.widgetContent_.html(), "widgetContent HTML in saving()");

    value = this.widgetContent_.attr("value");
    if(value !== undefined)
    {
        // some widget save their current state as the "value" attribute
        // so we can quickly retrieve that and return it as the result
        // (note that in this case we even skip calling the widget type
        // saving() function since the value is already the final result.)
        data.result = snapwebsites.castToString(value, "widgetContent value attribute");
    }
    else
    {
        // by default the result (what is sent to the server) is the
        // same as the HTML data, but widgets are free to chnage that
        // value (i.e. checkmark widgets set it to 0 or 1).
        //
        // TBD: should we also remove <br> tags with attributes?
        // TBD: should we remove <hr> tags at the start/end too?
        data.result = snapwebsites.castToString(
                            data.html.replace(/^(<br *\/?>| |\t|\n|\r|\v|\f|&nbsp;|&#160;|&#xA0;)+/, "")
                                     .replace(/(<br *\/?>| |\t|\n|\r|\v|\f|&nbsp;|&#160;|&#xA0;)+$/, ""),
                            "data html trimmed"
                      );
        data.result = data.result.replace(/<br *\/?>/g, "<br/>")
                                 .replace(/<hr *\/?>/g, "<hr/>");

        this.widgetType_.saving(this, data);
    }

    return data;
};


/** \brief Call after a widget was saved.
 *
 * This function reset the original data and modified flags when called.
 * It is expected to be called on a successful save.
 *
 * @param {snapwebsites.EditorWidgetTypeBase.SaveData} data  The data that was
 *        saved (what saving() returned.)
 *
 * @return {boolean}  Whether the widget was modified (true) or not
 *                    (false).
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.saved = function(data)
{
    this.originalData_ = data.html;
    this.originalValue_ = data.result;
    return this.wasModified(true);
};


/** \brief Generally called when you want to ignore changes in a widget.
 *
 * This function resets the original data from the current data. This is
 * useful to pretend that the widget was saved or you want to ignore the
 * changes.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.discard = function()
{
    this.originalData_ = snapwebsites.castToString(this.widgetContent_.html(), "widgetContent HTML in EditorWidget constructor for " + this.name_);

    // we do NOT want a "real" castToString() call on the value attribute
    // because we expect "undefined" to be returned in many cases
    this.originalValue_ = /** @type {string} */ (this.widgetContent_.attr("value"));

    this.wasModified(true);
};


/** \brief Retrieve the editor form object.
 *
 * This function returns the editor form object.
 *
 * @return {snapwebsites.EditorForm} The editor form object.
 */
snapwebsites.EditorWidget.prototype.getEditorForm = function()
{
    return this.editorForm_;
};


/** \brief Retrieve the editor base object.
 *
 * This function returns the editor base object as passed to
 * the constructor.
 *
 * @return {snapwebsites.EditorBase} The editor base object.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.getEditorBase = function()
{
    return this.editorBase_;
};


/** \brief Set the focus to this widget if possible.
 *
 * This function is used to set the focus to this widget.
 */
snapwebsites.EditorWidget.prototype.focus = function()
{
    if(this.isEnable())
    {
        this.widgetContent_.focus();
    }
};


/** \brief Remove the focus from this widget if it has it.
 *
 * This function removes the focus from this widget if it happened to
 * be the one with the focus. It is used, for example, when the widget
 * gets disabled or hidden.
 */
snapwebsites.EditorWidget.prototype.blur = function()
{
    this.widgetContent_.blur();
};


/** \brief Show the widget.
 *
 * This function is used to show the widget. If there is a label defined
 * for that widget (i.e. with the for="..." attribute properly defined)
 * then the label gets hidden too.
 *
 * \todo
 * Add signatures as supported by jQuery (duration, complete), (options).
 */
snapwebsites.EditorWidget.prototype.show = function()
{
    var id = this.widget_.attr("field_name");

    this.widget_.show();

    if(id)
    {
        jQuery("label[for='" + id + "']").show();
    }
};


/** \brief Hide the widget.
 *
 * This function is used to hide the widget. If there is a label defined
 * for that widget (i.e. with the for="..." attribute properly defined)
 * then the label gets hidden too.
 *
 * \todo
 * Add signatures as supported by jQuery (duration, complete), (options).
 */
snapwebsites.EditorWidget.prototype.hide = function()
{
    var id = this.widget_.attr("field_name");

    this.widget_.hide();

    if(id)
    {
        jQuery("label[for='" + id + "']").hide();
    }
};


/** \brief Enable the widget.
 *
 * This function is used to enable the widget.
 *
 * The function simply adds or removes the "disabled" class.
 *
 * By default, if the \p state parameter is not specified, the function
 * enables the widget. If the state is specified and it is true, the
 * widget is also enabled. When the state is specified and is false,
 * then it adds the disabled class.
 *
 * \important
 * If the state is not of type Boolean, then it is ignored and the
 * widget is always enabled. You may convert any value to a Boolean
 * value using the logical not (!) operator.
 *
 * @param {boolean=} opt_state  The new state, if true (default) enable,
 *                              otherwise disable the widget.
 */
snapwebsites.EditorWidget.prototype.enable = function(opt_state)
{
    var editable = this.widgetContent_.attr("contenteditable");

    if(typeof opt_state === "boolean" && !opt_state)
    {
        this.widget_.addClass("disabled");

        // set contenteditable to false so we know we have to restore it later
        // (but only if it exists)
        if(editable != undefined && editable != "")
        {
            this.widgetContent_.attr("contenteditable", "false");
        }

        // make sure the focus is lost from this widget if it was still here
        // (it should not now that contenteditable is false)
        this.blur();
    }
    else
    {
        this.widget_.removeClass("disabled");

        // restore the contenteditable state to true if present
        if(editable != undefined && editable != "")
        {
            // restore contenteditable
            this.widgetContent_.attr("contenteditable", "true");
        }
    }
};


/** \brief Disable the widget.
 *
 * This function is used to disable the widget.
 *
 * The function calls enable() with the value false as the state.
 *
 * Note that the disable function does not accept a parameter.
 */
snapwebsites.EditorWidget.prototype.disable = function()
{
    this.enable(false);
};


/** \brief Check whether a widget is enable.
 *
 * This function checks whether the widget is currently enabled.
 *
 * @return {boolean}  true if the widget is enable, false otherwise.
 */
snapwebsites.EditorWidget.prototype.isEnable = function()
{
    return !this.widget_.hasClass("disabled");
};


/** \brief Retrieve the jQuery widget.
 *
 * This function returns the jQuery widget attached to this
 * editor widget.
 *
 * @return {jQuery} The jQuery widget as passed to the constructor.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.getWidget = function()
{
    return this.widget_;
};


/** \brief Retrieve the jQuery widget content.
 *
 * This function returns the jQuery widget attached to this
 * editor widget which holds the content of the field represented
 * by this widget.
 *
 * This is the child of the widget with class "editor-content".
 *
 * @return {jQuery} The jQuery widget content.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.getWidgetContent = function()
{
    return this.widgetContent_;
};


/** \brief Check whether the background value should be shown.
 *
 * This function checks whether the background value of this widget
 * should be shown which happens when the widget is empty.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.checkForBackgroundValue = function()
{
    this.widget_.children(".snap-editor-background").toggle(snapwebsites.EditorWidget.isEmptyBlock(this.widgetContent_.html()));
};


/** \brief Check whether a block of HTML is empty or not.
 *
 * This function checks the HTML in the specified \p html parameter
 * and after cleaning up the string, returns true if it ends up
 * being empty.
 *
 * @param {string|jQuery} html
 *
 * @return {boolean}  true if the string can be considered empty.
 *
 * @final
 */
snapwebsites.EditorWidget.isEmptyBlock = function(html) // static
{
//#ifdef DEBUG
    if(typeof html !== "string")
    {
        throw new Error("snapwebsites.EditorBase.isEmptyBlock() called with a parameter which is not a string (" + (typeof html) + ")");
    }
//#endif

    //
    // replace all the nothingness by ""
    // and see whether the result is the empty string
    //
    // WARNING: in the following, if html is empty on entry then
    //          the result is still true but just a match against
    //          the regex would return false on the empty string
    //
    return html.replace(/^(<br *\/?>| |\t|\n|\r|&nbsp;)+$/, "").length === 0;
};


/** \brief Retrieve the current value of this widget.
 *
 * This function returns the current value of the widget.
 * Note that some widget do not really have a string as a value, you may
 * want to check out the widget type you are handling to know what it
 * returns.
 *
 * This is a helper function that is a replacement to the following:
 *
 * \code
 *      // get the value at once
 *      var value = widget.getValue();
 *
 *      // get the value and raw data
 *      var data = widget.saving();
 *      var value = data.result;
 * \endcode
 *
 * @return {string}  The value of this widget.
 */
snapwebsites.EditorWidget.prototype.getValue = function()
{
    var data = this.saving();
    return data.result;
};


/** \brief Retrieve the current value as a Date object.
 *
 * This function returns the current value of the widget as a JavaScript
 * Date object.
 *
 * If a date is considered invalid, the function returns a Date which
 * returns true when checked against isNaN().
 *
 * \code
 *      var d = my_widget.getValueAsDate();
 *
 *      if(isNaN(d))
 *      {
 *          // date is not valid
 *          ...
 *      }
 *      else
 *      {
 *          // date is good
 *          ...
 *      }
 * \endcode
 *
 * The function makes sure to first strips all the tags from the
 * value, just in case (there is often a trailing \<br/> tag in
 * line edit widgets.)
 *
 * @return {Date}  The value of this widget as a Date object.
 */
snapwebsites.EditorWidget.prototype.getValueAsDate = function()
{
    return new Date(snapwebsites.stripAllTags(this.getValue()));
};


/** \brief Reset the data of a widget to the last saved value.
 *
 * This function copies the original data back in this widget.
 * It restores the widget to the state it was when the form
 * was loaded, it is expected to mark the widget as not modified
 * and show or hide the background value as required.
 *
 * @final
 *
 * @return {boolean}  true if the restore had an effect.
 *
 * \sa resetValue()
 * \sa setValue()
 */
snapwebsites.EditorWidget.prototype.restoreValue = function()
{
    var modified = false;

    // We cannot directly use the setValue() with the original data
    // may have nothing to do with a user value. For example, a
    // dropdown may present the user with a sentence such as:
    // "Please select an option..." which is not a valid value.
    //
    // To properly restore, we thus have to restore that original
    // data and the value if any as two separate entities.
    //
    // Restore the original HTML of this widget
    //
    if(!this.widget_.hasClass("composite"))
    {
        // note that we ignore whether the original data of a composite
        // widget was modified
        //
        modified = this.widgetContent_.html() != this.originalData_;

        // the content of composite widget are other widgets and restoring
        // the whole thing could be a bad idea... (TBD)
        //
        this.widgetContent_.html(this.originalData_);
    }

    // if there was a value attribute, it is defined in the
    // originalValue_ parameter; this cannot be HTML like
    // the originalData_
    //
    if(this.originalValue_ !== undefined)
    {
        modified = modified || this.widgetContent_.attr("value") != this.originalValue_;
        this.widgetContent_.attr("value", this.originalValue_);
    }
    else
    {
        modified = modified || this.widgetContent_.attr("value") != "";
        this.widgetContent_.removeAttr("value");
    }

    // recheck the "was modified" flag; it should be set back to
    // false in pretty much all cases but we cannot be 100% sure
    // about that
    //
    this.wasModified(true);

    // finally, check whether the background value should be
    // displayed or not
    //
    this.checkForBackgroundValue();

    // return true if the restore did actual restore something
    //
    return modified;
};


/** \brief Reset the value of the widget.
 *
 * This function "resets" the value of this widget when changed is false.
 * This generally only works with widgets that have a form of default value.
 * The value itself does not get changed. Instead, the function makes it
 * look like the widget was not modified.
 *
 * If the parameters 'changed' is set to true then the widget is marked
 * as modified, otherwise the modified flag is not changed so if it was
 * already marked modified, it still is.
 *
 * To restore the content of a widget to what it was before the last time
 * it was saved, call the restoreValue() instead.
 *
 * \todo
 * Fix the name of this function because it is not really what you would
 * otherwise expect.
 *
 * @param {!boolean} changed  Whether to mark the widget as modified.
 *
 * @return {boolean}  true if the value got set.
 *
 * \sa restoreValue()
 */
snapwebsites.EditorWidget.prototype.resetValue = function(changed)
{
    var data;

    if(!this.widgetType_.resetValue(this))
    {
        // could not even change the value
        return false;
    }
    if(changed)
    {
        // recheck the "was modified" flag
        this.wasModified(true);
    }
    else
    {
        // programmer views this change as a 'current status' so we
        // prevent the wasModified() from being true in this case
        data = this.saving();
        this.saved(data);
    }
    this.checkForBackgroundValue();

    // it was set, whether it was modified is not our concern when returning
    return true;
};


/** \brief Set the value of the widget.
 *
 * This function sets the specified value as the new value of the widget.
 *
 * If the parameters 'changed' is set to true and the new value is
 * different from the existing value, then the widget is marked as
 * modified, otherwise the modified flag is not changed (it may
 * actually get reset if you restore the value in this way.)
 *
 * \todo
 * Note that at this time YOU are responsible to call the hasChanged()
 *
 * @param {!Object|string|number} value  The new widget value.
 * @param {!boolean} changed  Whether to mark the widget as modified.
 *
 * @return {boolean}  true if the value got set.
 */
snapwebsites.EditorWidget.prototype.setValue = function(value, changed)
{
    var data;

    if(!this.widgetType_.setValue(this, value))
    {
        // could not even change the value
        return false;
    }
    if(changed)
    {
        // recheck the "was modified" flag
        this.wasModified(true);
    }
    else
    {
        // programmer views this change as a 'current status' so we
        // prevent the wasModified() from being true in this case
        data = this.saving();
        this.saved(data);
    }
    this.checkForBackgroundValue();

    // it was set, whether it was modified is not our concern when returning
    return true;
};


/** \brief Get the type of this widget.
 *
 * This function returns the reference to the editor widget type.
 *
 * @return {snapwebsites.EditorWidgetTypeBase}
 */
snapwebsites.EditorWidget.prototype.getWidgetType = function()
{
    return this.widgetType_;
};


/** \brief Show a wait image inside this widget.
 *
 * This function creates a wait image inside the widget and then shows
 * it rotating.
 *
 * Any widget that needs to do a computation that will take time or
 * communicate with the server should probably use this function.
 *
 * \note
 * If the wait image is already visible, then nothing happens.
 */
snapwebsites.EditorWidget.prototype.showWaitImage = function()
{
    var that = this,
        w = this.getWidget();

    if(!this.waitWidget_)
    {
        w.prepend("<div class=\"widget-wait-image\"/>");
        this.waitWidget_ = w.children(".widget-wait-image");
    }

    if(isNaN(this.waitWidgetTimerID_))
    {
        this.waitWidget_.fadeIn(1000);
        this.waitWidgetTimerID_ = setInterval(
            function()
            {
                that.rotateWaitImage_();
            },
            200);
    }

    //
    // The rotateWait_ works but uses 100% of the CPU, so no good.
    // (especially for a wait!) At this time make use of a GIF instead.
    // The rotate is still very cool for a one time animation of 1
    // second or less.
    //
    // /** \brief A RotateAnimation object.
    //  *
    //  * This object is used to rotate the wait image by 30 degrees every 200ms.
    //  *
    //  * It gets created whenever a file is dropped and a preview is being
    //  * generated.
    //  *
    //  * @type {snapwebsites.RotateAnimation}
    //  * @private
    //  */
    // snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.rotateWait_ = null;
    //
    //if(!this.rotateWait_)
    //{
    //    wait_image = w.children(".widget-wait-image");
    //    this.rotateWait_ = new snapwebsites.RotateAnimation(wait_image, 30);
    //}
    //this.rotateWait_.start();
};


/** \brief Once the wait is over, hide it.
 *
 * This function is used to hide the wait image in this widget. It also
 * makes sure to stop the animation.
 */
snapwebsites.EditorWidget.prototype.hideWaitImage = function()
{
    if(!isNaN(this.waitWidgetTimerID_))
    {
        clearTimeout(this.waitWidgetTimerID_);
        this.waitWidgetTimerID_ = NaN;
        this.waitWidget_.fadeOut(200);
    }
};


/** \brief Rotate the wait image by 30 degrees.
 *
 * Each time we rotate the wait image we turn it by 30 degrees. This
 * function increments a counter that wraps back to zero when it
 * reaches 12.
 *
 * The function changes the background position to give the effect that
 * the image rotates. The image itself is expected to include 12 pictures
 * each at 30 degrees interval in one row (y does not change, only x).
 *
 * On my computer this animation takes less than 7% of the CPU. So I think
 * it is still acceptable.
 *
 * @private
 */
snapwebsites.EditorWidget.prototype.rotateWaitImage_ = function()
{
    this.waitWidgetPosition_ = (this.waitWidgetPosition_ + 1) % 12;
    this.waitWidget_.css('background-position', (this.waitWidgetPosition_ * -64) + 'px 0');
};


/** \brief Retrieve an object from this editor widget.
 *
 * This function checks whether a named block of data exists in the
 * widget data storage. If so it gets returned. Otherwise the function
 * returns undefined.
 *
 * @param {string} name  The name of the object to retrieve.
 *
 * @return {Object|boolean|number|string|null}  The object as saved by
 *                   setData() with the same name or undefined.
 *
 * \sa setData()
 */
snapwebsites.EditorWidget.prototype.getData = function(name)
{
    return this.widgetData_[name];
};


/** \brief Save data in this editor widget.
 *
 * This function is used to save an object of data in this widget.
 * It can later be retrieved with the use of the getData() function
 * and the exact same name.
 *
 * @param {string} name  The name of the object to save.
 * @param {Object|boolean|number|string|null|undefined} data  The actual data
 *                                                            to save.
 *
 * \sa getData()
 */
snapwebsites.EditorWidget.prototype.setData = function(name, data)
{
    // when setting to undefined, we actually want to delete the property
    // (i.e. setting a variable to undefined is not equivalent to deleting it)
    if(data === undefined)
    {
        delete this.widgetData_[name];
    }
    else
    {
        this.widgetData_[name] = data;
    }
};



/** \brief Snap EditorFormBase constructor.
 *
 * The EditorForm inherits the EditorFormBase.
 *
 * \code
 * class EditorFormBase extends ServerAccessCallbacks
 * {
 * public:
 *      function EditorFormBase(editor_base: EditorBase, form_widget: jQuery) : EditorFormBase;
 *      function getEditorBase() : EditorBase;
 *      function getFormWidget() : jQuery;
 *      virtual function saveData(mode: string, opt_option: object);
 *
 *      static const SAVE_MODE_PUBLISH: string;
 *      static const SAVE_MODE_SAVE: string;
 *      static const SAVE_MODE_SAVE_NEW_BRANCH: string;
 *      static const SAVE_MODE_SAVE_DRAFT: string;
 *
 * private:
 *      var editorBase_: EditorBase;
 *      var formWidget_: jQuery;
 * };
 * \endcode
 *
 * @param {snapwebsites.EditorBase} editor_base  The base editor object.
 * @param {jQuery} form_widget  The editor form DOM in a jQuery object.
 *
 * @return {!snapwebsites.EditorFormBase}  The newly created object.
 *
 * @extends {snapwebsites.ServerAccessCallbacks}
 * @constructor
 * @struct
 */
snapwebsites.EditorFormBase = function(editor_base, form_widget)
{
    snapwebsites.EditorFormBase.superClass_.constructor.call(this);

    this.editorBase_ = editor_base;
    this.formWidget_ = form_widget;

    return this;
};


/** \brief The EditorFormBase derives from ServerAccessCallbacks.
 *
 * This class inherits from the ServerAccessCallbacks so it can receive
 * callback events after a AJAX object was sent to the server.
 */
snapwebsites.inherits(snapwebsites.EditorFormBase, snapwebsites.ServerAccessCallbacks);


/** \brief Save the data and then publish it.
 *
 * When saving data to the database, it creates a new revision nearly
 * every single time (there is one case where it does not, but that's
 * probably a bug...)
 *
 * When the user publishes, that new revision becomes the current
 * revision. Otherwise the current revision does not change. This
 * allows for revisions to be checked and eventually corrected
 * before publication of the content.
 *
 * @type {string}
 * @const
 */
snapwebsites.EditorFormBase.SAVE_MODE_PUBLISH = "publish";


/** \brief Save the data in a new revision.
 *
 * Save the data entered in the page to a new revision entry. The
 * new entry is available for review, but it does not get shown
 * to people who could not edit the page in some way.
 *
 * Once everyone approaved of a page, the page can get published by
 * asking the system to show that specific revision.
 *
 * @type {string}
 * @const
 */
snapwebsites.EditorFormBase.SAVE_MODE_SAVE = "save";


/** \brief Create a new branch.
 *
 * This function saves the current data in a new branch. One can
 * create a new branch to make changes to the page such as adding
 * or removing tags and yet leave the current branch as the publicly
 * visible data.
 *
 * This gives the user time to work on his new content until he's
 * ready to publish it.
 *
 * @type {string}
 * @const
 */
snapwebsites.EditorFormBase.SAVE_MODE_SAVE_NEW_BRANCH = "save-new-branch";


/** \brief Save the data as a draft.
 *
 * Snap supports drafts to save the data being typed (in case the
 * browser or even the whole client computer crashes, the electricity
 * goes out, etc.)
 *
 * Drafts are special in that they get saved in a special location
 * instead of the normal revision.
 *
 * @type {string}
 * @const
 */
snapwebsites.EditorFormBase.SAVE_MODE_SAVE_DRAFT = "save-draft";


/** \brief A reference to the base editor object.
 *
 * This value is a reference to the base editor object so the
 * EditorForm objects can access it.
 *
 * @type {snapwebsites.EditorBase}
 * @private
 */
snapwebsites.EditorFormBase.prototype.editorBase_ = null;


/** \brief The jQuery object representing this form.
 *
 * This variable represents the DOM form as a jQuery object. It is
 * given to the EditorForm on creation.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorFormBase.prototype.formWidget_ = null;


/** \brief Retrieve the editor base object.
 *
 * This function returns the editor base object as passed to
 * the constructor.
 *
 * @return {snapwebsites.EditorBase} The editor base object.
 * @final
 */
snapwebsites.EditorFormBase.prototype.getEditorBase = function()
{
    return this.editorBase_;
};


/** \brief Retrieve the jQuery form widget.
 *
 * This function returns the form widget.
 *
 * @return {jQuery}
 * @final
 */
snapwebsites.EditorFormBase.prototype.getFormWidget = function()
{
    return this.formWidget_;
};


/*jslint unparam: true */
/** \brief Save the form data.
 *
 * This function is called to save the data. It generally happens in
 * two cases:
 *
 * \li When the user clicks on of the submit button (Publish, Save,
 *     Save New Branch, and Save Draft);
 * \li When a timer times out and the auto-save to draft feature is
 *     allowed.
 *
 * The supported modes are defined as constants:
 *
 * \li SAVE_MODE_PUBLISH
 * \li SAVE_MODE_SAVE
 * \li SAVE_MODE_SAVE_NEW_BRANCH
 * \li SAVE_MODE_SAVE_DRAFT
 *
 * @throws {Error} The function throws an error if the base class
 *                 version is called (i.e. not implemented.)
 *
 * @param {string} mode  The mode used to save the data.
 * @param {Object=} opt_options  Additional options to be added to the query
 *                               string.
 */
snapwebsites.EditorFormBase.prototype.saveData = function(mode, opt_options) // virtual
{
    throw new Error("snapwebsites.EditorFormBase.saveData() was called which means it was not properly overridden");
};
/*jslint unparam: false */



/** \brief Dialog used to show different Save options.
 *
 * By default the editor offers a simple save dialog which includes
 * several buttons used to save the changes to a page. The offered
 * buttons are:
 *
 * \li Publish -- save the current data and make it an official revision
 * \li Save -- save the changes, but do not make it the official revision
 * \li Save New Branch -- save the changes in a new branch
 * \li Save Draft -- save the data as a draft (no revision in this case)
 *
 * Editor forms used as standard form most often override the default
 * DOM buttons with just a Save button which pretty much acts like
 * the Publish button.
 *
 * \code
 * class EditorSaveDialog
 * {
 * public:
 *      function setPopup(widget: Element|jQuery, hide_button: boolean := true);
 *      function open();
 *      function close();
 *      function setStatus(new_status: boolean);
 *
 * private:
 *      function create_();
 *
 *      var editorForm_: EditorFormBase;
 *      var saveDialogPopupAutoHide_: boolean;
 *      var saveDialogPopup_: jQuery;
 * };
 * \endcode
 *
 * \todo
 * The branch the user decided to edit (i.e. with the query string
 * ...?a=edit&revision=1.2) needs to be taken in account as well.
 *
 * @param {snapwebsites.EditorFormBase}  editor_form The editor form that
 *                                       created this editor save dialog.
 *
 * @constructor
 */
snapwebsites.EditorSaveDialog = function(editor_form)
{
    this.editorForm_ = editor_form;

    return this;
};


/** \brief Mark EditorBase as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.EditorSaveDialog);


/** \brief An editor form reference.
 *
 * This parameter holds the editor form that created this save dialog.
 * It is used to call the saveData() function on the correct editor
 * form.
 *
 * @type {snapwebsites.EditorFormBase}
 *
 * @private
 */
snapwebsites.EditorSaveDialog.prototype.editorForm_ = null;


/** \brief Whether the saveDialogPopup_ widget gets hidden.
 *
 * This parameter holds a boolean value which tells us whether the
 * saveDialogPopup_ object should automatically be hidden or not.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorSaveDialog.prototype.saveDialogPopupAutoHide_ = true;


/** \brief Whether the saveDialogPopup_ widget is open.
 *
 * This parameter is true if the open() function was called. It is
 * set back to false when close() gets called.
 *
 * It is used so the setPopup() function knows whether to show or
 * hide the dialog / button that it is asked to manage it at the time
 * it gets called.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorSaveDialog.prototype.saveDialogPopupOpen_ = false;


/** \brief The jQuery of the DOM representing the save dialog.
 *
 * This parameter holds the jQuery object referencing the DOM representing
 * the save dialog.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorSaveDialog.prototype.saveDialogPopup_ = null;


/** \brief Create the default save dialog DOM.
 *
 * This function creates the default dialog DOM parameters. If you
 * call the setDialogDOM() function before the dialog needs to
 * be opened, then the default DOM will never get used.
 *
 * @private
 */
snapwebsites.EditorSaveDialog.prototype.create_ = function()
{
    var that = this,
        html = "<div id='snap_editor_save_dialog'>"
            + "<h3 class='title'>Editor</h3>"
            + "<div id='snap_editor_save_dialog_page'>"
            + "<p class='description'>You made changes to your page. Make sure to save your modifications.</p>"
            // this is wrong at this point because the current branch
            // management is more complicated...
            // (i.e. if you are editing a new branch that is not
            //       public then Publish would make that branch
            //       public and the Save would make that too?!)
            + "<p class='snap_editor_publish_p'><a class='button' id='snap_editor_publish' href='#'>Publish</a></p>"
            + "<p class='snap_editor_save_p'><a class='button' id='snap_editor_save' href='#'>Save</a></p>"
            + "<p class='snap_editor_save_new_branch_p'><a class='button' id='snap_editor_save_new_branch' href='#'>Save New Branch</a></p>"
            + "<p class='snap_editor_save_draft_p'><a class='button' id='snap_editor_save_draft' href='#'>Save Draft</a></p>"
            + "</div></div>";

    jQuery(html).appendTo("body");

    this.saveDialogPopup_ = jQuery("#snap_editor_save_dialog");

    // very simple positioning at this point
    this.saveDialogPopup_.css("left", jQuery(window).outerWidth(true) - 190);

    jQuery("#snap_editor_publish")
        .click(function(e)
            {
                e.preventDefault();
                that.editorForm_.saveData(snapwebsites.EditorFormBase.SAVE_MODE_PUBLISH);
            });
    jQuery("#snap_editor_save")
        .click(function(e)
            {
                e.preventDefault();
                that.editorForm_.saveData(snapwebsites.EditorFormBase.SAVE_MODE_SAVE);
            });
    jQuery("#snap_editor_save_new_branch")
        .click(function(e)
            {
                e.preventDefault();
                alert("Save New Branch! (not implemented yet)");
            });
    jQuery("#snap_editor_save_draft")
        .click(function(e)
            {
                e.preventDefault();
                that.editorForm_.saveData(snapwebsites.EditorFormBase.SAVE_MODE_SAVE_DRAFT);
            });

    // while creating a new page, the page is kept under "admin/drafts"
    // and in that case "save" and "save new branch" do not make sense
    //
    if(jQuery("meta[name='path']").attr("content") === "admin/drafts")
    {
        jQuery(".snap_editor_save_p").hide();
        jQuery(".snap_editor_save_new_branch_p").hide();
    }
};


/** \brief Define the save dialog popup to use for this dialog.
 *
 * In many cases you will define your own buttons for a dialog.
 * This function let you do that. Note that the editor expects those
 * to be hidden by default. It will show them whenever necessary
 * (i.e. when something changed.)
 *
 * \note
 * This function forces an immediate hide on the save popup dialog.
 *
 * \todo
 * Offer ways to just disable buttons (instead of hiding them)
 * and to keep the buttons "active" but generate errors if clicked
 * while currently saving.
 *
 * @param {Element|jQuery} widget  A jQuery selector.
 * @param {boolean=} opt_hide_button  If true (default) then hide this button.
 */
snapwebsites.EditorSaveDialog.prototype.setPopup = function(widget, opt_hide_button)
{
    // in case we already have a saveDialogPopup_, get rid of it
    //
    if(this.saveDialogPopup_)
    {
        if(this.saveDialogPopup_.attr("id") == "snap_editor_save_dialog")
        {
            // this is the default created by the create_() function, we
            // can just remove it, no need to keep it around
            //
            this.saveDialogPopup_.remove();
        }
        else
        {
            // this was a user defined widget, so leave it alone but still
            // hide it so it does not stay on the screen
            //
            this.saveDialogPopup_.hide();
        }
    }

    this.saveDialogPopup_ = jQuery(widget);
    this.saveDialogPopupAutoHide_ = opt_hide_button === undefined || opt_hide_button;
    if(this.saveDialogPopupAutoHide_)
    {
        this.saveDialogPopup_.toggle(this.saveDialogPopupOpen_);
    }
};


/** \brief Open the save dialog.
 *
 * This function is generally called whenever the user makes a change
 * that needs to be sent to the server.
 *
 * First the function makes sure that a dialog is defined, if not it
 * initializes the default dialog.
 *
 * Then it shows the dialog.
 *
 * In some rare cases, it may be useful to show the block as an
 * "inline-block" (or some other display mode) instead of the default
 * of "block". When that is required, define a data attribute named
 * "data-display-mode" and set its value to the mode you want to use.
 *
 * \note
 * This function does not change the dialog buttons status. The
 * save function is expected to call the saveDialogStatus() function
 * for that purpose.
 *
 * \todo
 * The positioning of the dialog is done at creation time so it can be
 * a problem if it is expected (for example) to not overlay the area
 * being edited.
 */
snapwebsites.EditorSaveDialog.prototype.open = function()
{
    var mode;

    if(!this.saveDialogPopup_)
    {
        this.create_();
    }
    if(this.saveDialogPopupAutoHide_)
    {
        // user can specify the display mode (in most cases it should be
        // "block", the default, or "inline-block", although it could be
        // something else like just "inline".)
        //
        mode = this.saveDialogPopup_.data("display-mode");
        if(mode === undefined)
        {
            // default to "block"
            //
            mode = "block";
        }
        this.saveDialogPopup_.fadeIn(300).css("display", mode);
    }

    this.saveDialogPopupOpen_ = true;
};


/** \brief Close the save dialog popup.
 *
 * Once a save completed, the editor form checks whether other
 * modifications were performed while saving, if not, then the
 * save dialog gets closed.
 *
 * \note
 * This function does not change the status of the dialog buttons.
 * The save function is expected to call the setStatus() function
 * for that purpose.
 */
snapwebsites.EditorSaveDialog.prototype.close = function()
{
    if(this.saveDialogPopup_ && this.saveDialogPopupAutoHide_)
    {
        this.saveDialogPopup_.fadeOut(300);
    }
    // else throw?

    this.saveDialogPopupOpen_ = false;
};


/** \brief Setup the save dialog status.
 *
 * When a user clicks on a save dialog button, you should call this
 * function to disable the dialog.
 *
 * \warning
 * The function throws if the popup is not yet defined. You should not
 * be able to save without the dialog having been created so that
 * should not happen. That being said, it means we cannot call this
 * function before we called the open() function.
 *
 * @param {boolean} new_status  Whether the widget is enabled (true)
 *                              or disabled (false).
 */
snapwebsites.EditorSaveDialog.prototype.setStatus = function(new_status)
{
//#ifdef DEBUG
    // dialog even exists?
    if(!this.saveDialogPopup_)
    {
        throw new Error("EditorSaveDialog.setStatus() called without a save dialog popup defined.");
    }
//#endif

    if(new_status)
    {
        this.saveDialogPopup_.parent().children("a").removeClass("disabled");
    }
    else
    {
        this.saveDialogPopup_.parent().children("a").addClass("disabled");
    }
};



/** \brief Snap EditorForm constructor.
 *
 * \code
 * class EditorForm extends EditorFormBase
 * {
 * public:
 *      function EditorForm() : EditorForm;
 *      function getName() : string;
 *      function getWidgetByName(name: string) : EditorWidget;
 *      function getSession() : string;
 *      function setInPopup(in_popup: boolean);
 *      function getToolbarAutoVisible() : boolean;
 *      function setToolbarAutoVisible(toolbar_auto_visible: boolean);
 *      virtual function saveData(mode: string, opt_options: object);
 *      virtual function serverAccessSuccess(result: ResultData);
 *      virtual function serverAccessError(result: ResultData);
 *      virtual function serverAccessComplete(result: ResultData);
 *      function isSaving() : boolean;
 *      function setSaving(new_status: boolean, will_redirect: boolean);
 *      function changed();
 *      function getSaveDialog() : EditorSaveDialog;
 *      function getDefaultButton() : jQuery;
 *      function resetTimeout();
 *      function formTimedout();
 *      function setTimedoutCallback(f: function(EditorForm));
 *      function resetAutoReset(force: boolean);
 *      function formAutoReset();
 *      function setAutoResetCallback(f: function(EditorForm));
 *      function newTypeRegistered();
 *      function wasModified(recheck: boolean) : boolean;
 *      static function beforeHide(popup: Popup);
 *      function earlyClose() : boolean;
 *      static function titleToURI(title: string) : string;
 *
 * private:
 *      function readyWidgets_();
 *      function readyForm_();
 *
 *      var usedTypes_: Object;                 // map of types necessary to open that form
 *      var session_: string;                   // session identifier
 *      var name_: string;                      // the name of the form
 *      var widgets_: jQuery;                   // the widgets of this form
 *      var editorWidgets_: Object;             // map of editor objects
 *      var widgetInitialized_: boolean;        // whether the form was initialized
 *      var saveDialog_: EditorSaveDialog;      // a reference to the save dialog
 *      var mode_: string;                      // general mode the form is used as
 *      var toolbarAutoVisible: boolean;        // whether to show the toolbar automatically
 *      var modified_: boolean;                 // one or more fields changed
 *      var saveFunctionOnSuccess_: function(editor_form: EditorForm, result: snapwebsites.ServerAccessCallbacks.ResultData);
 *                                              // function called in case the save succeeded
 *      var saveFunctionOnError_: function(editor_form: EditorForm, result: snapwebsites.ServerAccessCallbacks.ResultData);
 *                                              // function called in case the save failed
 *      var savedData_: Object;                 // a set of objects to know whether things changed while saving
 *      var serverAccess_: ServerAccess = null; // a ServerAccess object to send the AJAX
 * };
 * \endcode
 *
 * \note
 * The Snap! EditorForm objects are created as required based on the DOM.
 * If the DOM is dynamically updated to add more forms, then it may require
 * special handling (TBD at this point) to make sure that the new forms
 * are handled by the editor.
 *
 * @param {snapwebsites.EditorBase} editor_base  The base editor object.
 * @param {jQuery} form_widget  The editor form DOM in a jQuery object.
 * @param {!string} name  The name of the editor form.
 * @param {!string} session  The form session identification.
 *
 * @return {!snapwebsites.EditorForm}  The newly created object.
 *
 * @extends {snapwebsites.EditorFormBase}
 * @constructor
 * @struct
 */
snapwebsites.EditorForm = function(editor_base, form_widget, name, session)
{
    var mode;

    snapwebsites.EditorForm.superClass_.constructor.call(this, editor_base, form_widget);

    this.editorWidgets_ = {};
    this.usedTypes_ = {};
    this.session_ = session;
    this.name_ = name;
    mode = form_widget.attr("mode");
    if(mode) // user specified?
    {
        this.mode_ = snapwebsites.castToString(mode, "casting the editor form mode");
    }
    else
    {
        this.mode_ = "default";
    }

    this.readyWidgets_();

    return this;
};


/** \brief EditorForm inherits from EditorFormBase.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorForm, snapwebsites.EditorFormBase);


/** \brief A map of widget types used by this form.
 *
 * This object represents a list of types that this form uses to handle
 * its widgets. Each type defines functions to handle that widget
 * as it needs to be handled and allows for easy extensions, etc.
 *
 * The map uses the name of the type as the index and true or false
 * as the value. true is used when the type is not known to be
 * provided by the system, false once it is known to be defined.
 *
 * @type {!Object}
 * @private
 */
snapwebsites.EditorForm.prototype.usedTypes_; // = {}; -- initialized in the constructor to avoid problems


/** \brief The session identification of this form.
 *
 * This is the session identification and random number for this editor
 * form.
 *
 * @type {!string}
 * @private
 */
snapwebsites.EditorForm.prototype.session_ = "";


/** \brief The name of this form.
 *
 * This is the name of this editor form object. The name is taken from the
 * "form_name" attribute which must exist on the "editor-form" DOM element.
 *
 * @type {!string}
 * @private
 */
snapwebsites.EditorForm.prototype.name_ = "";


/** \brief A jQuery array of widgets found in this form.
 *
 * This parameter is the jQuery array of widgets defined in this
 * form.
 *
 * \bug
 * At this time, dynamically adding or removing widgets is not supported.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorForm.prototype.widgets_ = null;


/** \brief A jQuery array of widgets found in this form.
 *
 * This parameter is the map of EditorWidget objects defined in this
 * form. The key used to define these widget is the name of the widget
 * (the parameter used as the name.) This is quite practical to retrieve
 * widgets which name is known.
 *
 * \bug
 * At this time, dynamically adding or removing widgets is not supported.
 *
 * @type {Object.<snapwebsites.EditorWidget>}
 * @private
 */
snapwebsites.EditorForm.prototype.editorWidgets_ = null;


/** \brief Flag to know whether the widgets got initialized.
 *
 * We initialize the widgets as soon as we know that all the widget
 * types were loaded. At that point we know that the entire form
 * can be properly initialized.
 *
 * Once the flag is true, the form is fully initialized and
 * functional.
 *
 * @type {!boolean}
 * @private
 */
snapwebsites.EditorForm.prototype.widgetInitialized_ = false;


/** \brief The popup dialog object.
 *
 * This member is the save dialog widget. It is an object
 * handling a few DOM widgets representing buttons used to
 * let the user send his work to the server.
 *
 * @type {snapwebsites.EditorSaveDialog}
 * @private
 */
snapwebsites.EditorForm.prototype.saveDialog_ = null;


/** \brief The Darken Screen used while saving editor data.
 *
 * This reference is used to keep track of a DarkenScreen while
 * saving data from the editor objects.
 *
 * @type {snapwebsites.DarkenScreen}
 * @private
 */
snapwebsites.EditorForm.prototype.saveDarkenScreen_ = null;


/** \brief The mode used for this editor form.
 *
 * This parameter is taken from the attribute named "mode" of the
 * div representing the editor form. The mode can be set to one of
 * the following values:
 *
 * \li default -- process the form "as normal", you do not need to define
 *                this value as it is the default.
 * \li save-all -- save all the fields whether or not they were modified.
 *
 * @type {!string}
 * @private
 */
snapwebsites.EditorForm.prototype.mode_ = "";


/** \brief Whether the toolbar is shown immediately on focus.
 *
 * This flag is used to know whether the toolbar should be shown on
 * focus. If so, the value is true (the default). You may turn this
 * value off using the setToolbarAutoVisible() function.
 *
 * @type {!boolean}
 * @private
 */
snapwebsites.EditorForm.prototype.toolbarAutoVisible_ = true;


/** \brief Whether this form was modified.
 *
 * Whenever a form is first loaded and after a successful save,
 * this flag is set to false, meaning that the form was not
 * modified. Each time an event that may have modified the
 * form happens, the wasModified() function is called to check
 * all the fields. If one or more were modified, then the form
 * is marked as modified.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorForm.prototype.modified_ = false;


/** \brief A function to call on a successful save.
 *
 * This reference points to a function that gets called in the event
 * of a successful save. You may define one such function.
 *
 * @type {?function(snapwebsites.EditorForm, snapwebsites.ServerAccessCallbacks.ResultData)}
 * @private
 */
snapwebsites.EditorForm.prototype.saveFunctionOnSuccess_ = null;


/** \brief A function to call on a successful save.
 *
 * This reference points to a function that gets called in the event
 * of a failed save. You may define one such function.
 *
 * @type {?function(snapwebsites.EditorForm, snapwebsites.ServerAccessCallbacks.ResultData)}
 * @private
 */
snapwebsites.EditorForm.prototype.saveFunctionOnError_ = null;


/** \brief An object of the data as sent to the server.
 *
 * The user clicks Save, that may take a little time since the entier
 * block of data is sent to the server (maybe at some point we'll have
 * a JavaScript "diff" function...). While that happens, some pages do
 * not block the user who can continue to edit his work. When that
 * happens the behavior of the editor changes slightly. To know whether
 * such a change happens, the data is saved in this object for the time
 * it takes to process the save command.
 *
 * @type {Object}
 * @private
 */
snapwebsites.EditorForm.prototype.savedData_ = null;


/** \brief A server access object.
 *
 * Whenever the user wants to save his data, a ServerAccess object
 * is required so we create one and keep its reference here. It
 * will be reused on each subsequent save (instead of a new one
 * created each time.)
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.EditorForm.prototype.serverAccess_ = null;


/** \brief A callback to call when the form times out.
 *
 * This variable holds a reference to a function that the form
 * timeout function calls whenever the form times out.
 *
 * @type {?function(snapwebsites.EditorForm)}
 * @private
 */
snapwebsites.EditorForm.prototype.formTimedoutCallback_ = null;


/** \brief The identifier of the timeout used to listen to.
 *
 * This number represents the identifier of the timeout object
 * in use to timeout the form. The function gets called once
 * and only if it times out.
 *
 * If the timer is not setup, then the value is NaN.
 *
 * This number should always get set in the form initialization
 * process since all form sessions time out at some point.
 *
 * @type {number}
 * @private
 */
snapwebsites.EditorForm.prototype.formTimeoutId_ = NaN;


/** \brief A callback to call when the auto-reset feature times out.
 *
 * This variable holds a reference to a function that the
 * formAutoReset() function calls whenever the auto-reset times
 * out. This may happen multiple times.
 *
 * The function called receives no parameters and returns nothing.
 * You can only define one such function per form.
 *
 * @type {?function(snapwebsites.EditorForm)}
 * @private
 */
snapwebsites.EditorForm.prototype.formAutoResetCallback_ = null;


/** \brief The identifier of the auto-reset used to listen to.
 *
 * This number represents the identifier of the auto-reset object
 * (i.e. timer) in use to reset the form back to its default
 * values.
 *
 * If the timer is not setup, then the value is NaN.
 *
 * Most forms should not define the auto-reset parameter, especially
 * forms that require the user to be logged in to be accessible.
 *
 * @type {number}
 * @private
 */
snapwebsites.EditorForm.prototype.formAutoResetId_ = NaN;


/** \brief Retrieve the editor form name.
 *
 * This function returns the name of the editor form.
 *
 * @return {string} The editor form name.
 */
snapwebsites.EditorForm.prototype.getName = function()
{
    return this.name_;
};


/** \brief Retrieve a widget reference.
 *
 * This function returns the EditorWidget using the widget's name.
 *
 * @param {string} name  The name of the widget to retrieve.
 *
 * @return {snapwebsites.EditorWidget} A reference to the editor widget.
 */
snapwebsites.EditorForm.prototype.getWidgetByName = function(name)
{
    return this.editorWidgets_[name];
};


/** \brief Retrieve the session of this EditorForm object.
 *
 * This function returns the session one can use to communicate with the
 * server about this form.
 *
 * @return {string} A session string of the editor widget.
 */
snapwebsites.EditorForm.prototype.getSession = function()
{
    return this.session_;
};


/** \brief Return whether the toolbar should automatically be shown.
 *
 * When clicking in a widget, a form can automatically show the
 * toolbar corresponding to that widget. This is true by default.
 * The toolbar can be shown / hidden using the Ctlr-T key as well.
 *
 * \todo
 * We'll also want to add a close button at some point
 *
 * @return {boolean} true if the toolbar should be shown on focus.
 */
snapwebsites.EditorForm.prototype.getToolbarAutoVisible = function()
{
    return this.toolbarAutoVisible_;
};


/** \brief Change whether the toolbar should automatically be shown.
 *
 * Whenever a widget gets the focus we can automatically have the
 * toolbar popup. By default, this is true. It can be changed to
 * false using this function.
 *
 * @param {boolean} toolbar_auto_visible  Whether the toolbar should be
 *                                        shown on widget focus.
 */
snapwebsites.EditorForm.prototype.setToolbarAutoVisible = function(toolbar_auto_visible)
{
    this.toolbarAutoVisible_ = toolbar_auto_visible;
};


/** \brief Massage the title to make it a URI.
 *
 * This function transforms the characters in \p title so it can be
 * used as a segment of the URI of this page. This is quite important
 * since we use the URI to save the page.
 *
 * @param {string} title  The title to tweak.
 *
 * @return {string}  The tweaked title. It may be an empty string.
 */
snapwebsites.EditorForm.titleToURI = function(title) // static
{
    // force all lower case
    title = title.toLowerCase();
    // replace spaces with dashes
    title = title.replace(/ +/g, "-");
    // remove all characters other than letters and digits
    title = title.replace(/[^-a-z0-9_]+/g, "");
    // remove duplicate dashes
    title = title.replace(/--+/g, "-");
    // remove dashes at the start & end
    title = title.replace(/^-+/, "");
    title = title.replace(/-+$/, "");

    return title;
};


/** \brief A function to call on a successful save.
 *
 * If defined, this function is called when the Save function
 * returned a successful result and is not asked to redirect the
 * user.
 *
 * This is particularly useful if you used a form in a popup that
 * should be closed on success.
 *
 * @param {function(snapwebsites.EditorForm, snapwebsites.ServerAccessCallbacks.ResultData)} f  A function to call on success.
 */
snapwebsites.EditorForm.prototype.setSaveFunctionOnSuccess = function(f)
{
    this.saveFunctionOnSuccess_ = f;
};


/** \brief A function to call on an error while saving.
 *
 * If defined, this function is called when the Save function
 * returned with a failed result.
 *
 * This is particularly useful if your C++ function generated some data
 * in the result structure and you want to make use of it.
 *
 * @param {function(snapwebsites.EditorForm, snapwebsites.ServerAccessCallbacks.ResultData)} f  A function to call on failure.
 */
snapwebsites.EditorForm.prototype.setSaveFunctionOnError = function(f)
{
    this.saveFunctionOnError_ = f;
};


/** \brief Function called on success.
 *
 * This function is called if the remote access was successful. The
 * result object includes a reference to the XML document found in the
 * data sent back from the server.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.EditorForm.prototype.serverAccessSuccess = function(result) // virtual
{
    var key,                    // loop index
        modified = false;       // whether some data was modified while saving

    // request for the existing messages to automatically get hidden if
    // none were returned from the server
    //
    result.hide_messages = true;

    snapwebsites.EditorForm.superClass_.serverAccessSuccess.call(this, result);

    // success! so it was saved and now that is the new original value
    // and next "Save" does not do anything
    for(key in this.editorWidgets_)
    {
        if(this.editorWidgets_.hasOwnProperty(key))
        {
            if(this.savedData_[key])
            {
                if(this.editorWidgets_[key].saved(this.savedData_[key]))
                {
                    modified = true;
                }
            }
        }
    }

    // if not modified while processing the POST, hide the save buttons
    if(!modified)
    {
        this.getSaveDialog().close();
    }

    // in case the manager of the form wants to know that a save was
    // successful (but only if we're not going to redirect the user)
    if(this.saveFunctionOnSuccess_
    && !snapwebsites.ServerAccess.willRedirect(result))
    {
        this.saveFunctionOnSuccess_(this, result);
    }

    // WARNING: the result of willRedirect() CANNOT be cached since
    //          the saveFunctionOnSuccess_() call may change it
    this.setSaving(false, snapwebsites.ServerAccess.willRedirect(result));
};


/** \brief Function called on error.
 *
 * This function is called if the remote access generated an error.
 * In this case errors include I/O errors, server errors, and application
 * errors. All call this function so you do not have to repeat the same
 * code for each type of error.
 *
 * \li I/O errors -- somehow the AJAX command did not work, maybe the
 *                   domain name is wrong or the URI has a syntax error.
 * \li server errors -- the server received the POST but somehow refused
 *                      it (maybe the request generated a crash.)
 * \li application errors -- the server received the POST and returned an
 *                           HTTP 200 result code, but the result includes
 *                           a set of errors (not enough permissions,
 *                           invalid data, etc.)
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.EditorForm.prototype.serverAccessError = function(result) // virtual
{
    snapwebsites.EditorForm.superClass_.serverAccessError.call(this, result);

    // in case the manager of the form wants to know that an error
    // occured and get the corresponding result information
    if(this.saveFunctionOnError_)
    {
        this.saveFunctionOnError_(this, result);
    }

    this.setSaving(false, false);
};


/** \brief Function called on completion.
 *
 * This function is called once the whole process is over. It is most
 * often used to do some cleanup.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.EditorForm.prototype.serverAccessComplete = function(result) // virtual
{
    // we do not need that data anymore, release it
    this.savedData_ = null;

    // manage messages if any
    snapwebsites.EditorForm.superClass_.serverAccessComplete.call(this, result);
};


/** \brief Save the data.
 *
 * This function checks whether the data changed or not. If it was
 * modified, or the form was setup to save all the widgets content
 * each time, then the function builds an object and sends the
 * data to the server using AJAX.
 *
 * @param {!string} mode  The mode used to save the data.
 * @param {Object=} opt_options  Additional options to be added to the query
 *                               string.
 */
snapwebsites.EditorForm.prototype.saveData = function(mode, opt_options)
{
    var key,                                    // loop index
        w,                                      // widget being managed
        obj = {},                               // object to send via AJAX
        save_all = this.mode_ === "save-all",   // whether all fields are sent to the server
        valid = true;                           // whether the data is valid and can be sent

    // are we already saving? if so, generate an error
    if(this.isSaving())
    {
        // TODO: translation support
        snapwebsites.OutputInstance.displayOneMessage(
                "Browser Busy",
                "You already clicked one of these buttons. Please wait until the save is over.");
        return;
    }

    this.savedData_ = {};
    for(key in this.editorWidgets_)
    {
        if(this.editorWidgets_.hasOwnProperty(key))
        {
            // note that widgets marked as "immediate-save" are ignored
            // here because those were saved as soon as they were modified
            // so there is no need for us to save them again here (plus
            // in some cases it would be impossible like for file upload)
            w = this.editorWidgets_[key];

            // WARNING: we do "validate() && valid" in that order because
            //          placed this way the validate() gets called whether
            //          the 'valid' variable is true or false
            //
            valid = w.validate() && valid;

            if(valid)
            {
                if((save_all || w.wasModified(true) || w.getWidget().hasClass("always-save"))
                && !w.getWidget().hasClass("immediate-save"))
                {
                    // once saved once in this round, make sure to always save
                    // until we get a successful save otherwise data saved in
                    // the draft may wrongly get used!
                    w.getWidget().addClass("always-save");

                    this.savedData_[key] = w.saving();
                    obj[key] = this.savedData_[key].result;
                }
            }
        }
    }
    if(!valid)
    {
        return;
    }

    // mark the form as saving, it may use CSS to show the new status
    this.setSaving(true, false);

    // this test is not 100% correct for the Publish or Create Branch
    // buttons...
    if(!jQuery.isEmptyObject(obj))
    {
        obj._editor_save_mode = mode;
        obj._editor_session = this.session_;
        if(this.editorWidgets_.hasOwnProperty("title"))
        {
            obj._editor_uri = snapwebsites.EditorForm.titleToURI(this.editorWidgets_.title.saving().result);
        }
        if(!this.serverAccess_)
        {
            this.serverAccess_ = new snapwebsites.ServerAccess(this);
        }
        this.serverAccess_.setURI(snapwebsites.castToString(jQuery("link[rel='page-uri']").attr("href"), "casting href of the page-uri link to a string in snapwebsites.EditorForm.saveData()"),
                                  opt_options);
        this.serverAccess_.setData(obj);
        this.serverAccess_.send();
    }
    else
    {
        this.setSaving(false, false);

        // the serverAccess callback would otherwise call the close
        this.getSaveDialog().close();
    }
};


/** \brief Check whether this form is currently saving data.
 *
 * To avoid problems we prevent the editor from saving more than once
 * simultaneously.
 *
 * @return {boolean}  true if the form is currently saving data.
 * @final
 */
snapwebsites.EditorForm.prototype.isSaving = function() // virtual
{
    return this.getFormWidget().is(".editor-saving");
};


/** \brief Set the current saving state of the form.
 *
 * This function set the saving state to "true" or "false". This is
 * marked using the "editor-saving" class in the editor form widget.
 * That can be used to show that the form is being saved by changing
 * a color or a border.
 *
 * @param {boolean} new_status  The saving status of this editor form.
 * @param {boolean} will_redirect  Whether we're going to be redirected just
 *                                 after this call (keep the darken page if
 *                                 true!)
 */
snapwebsites.EditorForm.prototype.setSaving = function(new_status, will_redirect)
{
    // TBD: use the will_redirect flag to know what else to do?
    //      (i.e. if will_redirect is true, we probably should not
    //      do anything here, what do you think?)
    //
    this.getFormWidget().toggleClass("editor-saving", new_status);
    this.saveDialog_.setStatus(!new_status);

    // TODO: add a condition coming from the DOM (i.e. we do not want
    //       to gray out the screen if the user is expected to be
    //       able to continue editing while saving)
    //
    //       said class="..." is nearly there (see header trying to assign
    //       body attributes), we will then need to test it here
    //
    //       WARNING: this needs to be moved to the editor-form object
    //                instead of the body! (i.e. one flag per form)
    //
    if(!will_redirect)
    {
        if(new_status)
        {
            if(!this.saveDarkenScreen_)
            {
                // darken the screen with a "Please wait ..." screen
                //
                this.saveDarkenScreen_ = new snapwebsites.DarkenScreen(150, true);
            }
        }
        else
        {
            if(this.saveDarkenScreen_)
            {
                // remove the darken screen
                //
                this.saveDarkenScreen_.close();
                this.saveDarkenScreen_ = null;
            }
        }
    }
};


/** \brief Let the form know that one of its widgets changed.
 *
 * This signal implementation is called whenever the form detects
 * that one of its widgets changed.
 *
 * In most cases it will show the "Save Dialog" although it may just
 * want to ignore the fact. (i.e. some forms do not care about changes
 * as their "save" button is always shown, for example, or it may
 * reacted on this event directly and not give the user a choice to
 * save the changes.)
 *
 * \todo
 * Support a way to tell others to react to this change.
 */
snapwebsites.EditorForm.prototype.changed = function()
{
    // tell others that something changed in the editor form
    var e = jQuery.Event("formchange",
        {
            form: this
        });
    this.getFormWidget().trigger(e);

    if(!this.getFormWidget().is(".no-save"))
    {
        this.getSaveDialog().open();
    }
};


/** \brief Retrieve the save dialog.
 *
 * This function retrieves a reference to the save dialog. It is a
 * function because the save dialog doesn't get created until
 * requested.
 *
 * \note
 * Unfortunately, if you want to define your own save buttons,
 * then you'll be creating the save dialog up front.
 *
 * @return {snapwebsites.EditorSaveDialog}  The save dialog reference.
 */
snapwebsites.EditorForm.prototype.getSaveDialog = function()
{
    if(!this.saveDialog_)
    {
        this.saveDialog_ = new snapwebsites.EditorSaveDialog(this);
    }
    return this.saveDialog_;
};


/** \brief Retrieve the default button if there is one.
 *
 * This function searches for the DOM object that has class
 * "editor-default-button". In most cases this is an anchor.
 *
 * The function may return an empty jQuery list.
 *
 * \note
 * If the function finds more than one default button, then it
 * returns the first that is not currently disabled.
 *
 * @return {jQuery}  The jQuery representation of the DOM object.
 */
snapwebsites.EditorForm.prototype.getDefaultButton = function()
{
    // search the list in default buttons for one that is not currently
    // disabled or hidden
    //
    // TODO: in debug mode we should check to see whether we have more than
    //       one answer because if so it is probably a bug (i.e. you cannot
    //       have more than one visible default button at a time)
    //
    // TODO: I need to check for disabled items; but for that we need to
    //       have an idea of how we mark an anchor as disabled and enforcing
    //       that state... (we want to switch to using button widgets and
    //       not direct anchors, but that is the next round.)
    //
    return this.getFormWidget().find(".editor-default-button").filter(":visible").first();
};


/** \brief Ready the widget for attachment.
 *
 * This function goes through the widgets and prepare them to be
 * attached to their respective handlers. Because we want to have
 * an extensible editor, this means we need to allow for "callbacks"
 * to be defined on the extension and not on some internal object.
 * This works by getting the list of widgets, retrieving their types,
 * and looking for a class of that name. If the class does not exist
 * (yet) then the form is not yet complete and the loader goes on.
 * Once all the types are properly defined, the form is ready and
 * the user can start editing.
 *
 * \note
 * The widgets themselves are marked with ".editor-content". The
 * editor always encapsulate those inside a div marked with
 * ".snap-editor". So there is a one to one correlation between
 * both of those.
 *
 * @private
 */
snapwebsites.EditorForm.prototype.readyWidgets_ = function()
{
    var that = this,
        form_widget = this.getFormWidget();

    // retrieve the widgets defined in this form
    //
    // note that we ignore any widgets that appear in a sub-form that would
    // be included in this form with the .not()
    this.widgets_ = form_widget.find(".snap-editor")
               .not(form_widget.find("* .editor-form .snap-editor"));

    // retrieve the field types for all the widgets
    this.widgets_.each(function(idx, w)
        {
            var type = jQuery(w).attr("field_type");
            that.usedTypes_[type] = true;
        });

    // check whether all the types are available
    // if so then the function finishes the initialization of the form
    this.newTypeRegistered();

    // make labels focus the corresponding editable box
    form_widget.find("label[for!='']").click(function(e)
        {
            if(!(jQuery(e.target).is("a")))
            {
                // the default may recapture the focus, so avoid it!
                e.preventDefault();
                e.stopPropagation();

                jQuery("div[name='" + jQuery(this).attr("for") + "']").focus();
            }
        });
};


/** \brief Finish up the form initialization.
 *
 * This function finishes up the global form initialization.
 * It includes the following:
 *
 * \li Setup a general timeout of the form. This timeout corresponds to
 *     the timeout of the form session.
 * \li Setup the form auto-reset feature. This is a timer that is used
 *     to automatically reset the form after a timeout delay.
 *
 * @private
 */
snapwebsites.EditorForm.prototype.readyForm_ = function()
{
    this.resetTimeout();
    this.resetAutoReset(true);
};


/** \brief Setup the timer to auto-disable this form.
 *
 * This function setups the timer used to know when to form session
 * times out. One the session of a form times out, sending the form
 * to the server fails. So there is no real need to send said form.
 *
 * This timeout is set once since the session duration does not
 * magically change. In other words, typing or moving things around
 * will not extend the session time out.
 *
 * Calling this function more than once has no effect on the second
 * call.
 */
snapwebsites.EditorForm.prototype.resetTimeout = function()
{
    // Note: the "timeout" attribute may include a redirect path as
    //       well, the parseFloat() ignores it...
    //
    var minutes = parseFloat(this.formWidget_.attr("timeout")),
        that = this;

    // already setup?
    if(!isNaN(this.formTimeoutId_))
    {
        return;
    }

    if(minutes <= 0 || isNaN(minutes))
    {
        // if undefined or invalid, use 24 hours, which is the
        // default session time defined in editor.cpp
        minutes = 1440;
    }

    // reduce by 3 minutes because the timing is always a bit off
    // between the client and the server
    minutes -= 3;
    if(minutes < 3)
    {
        // make sure we have a minimum
        minutes = 3;
    }

    this.formTimeoutId_ = setTimeout(function()
        {
            that.formTimedout();
        },
        minutes * 60000); // convert minutes to ms
};


/** \brief The form timed out.
 *
 * This function is called whenever the form timer is triggered.
 * The function makes sure to clear the timer (in case it gets
 * called before the timer itself is triggered,) and then clears
 * the contents and disable the widgets so end users cannot
 * edit the form anymore. It would be useless to let the users
 * still edit the content since the session timed out and
 * sending that content to the server would fail with a
 * session timed out error.
 *
 * You may call this function early to mark the form
 * as timed out, whether or not the timer event was
 * triggered.
 *
 * This function may be called more than once, although it
 * probably should not. There is no logic within this function
 * to know whether it was already called.
 *
 * It is possible to request the timeout to also redirect
 * the user (found in the \<timeout ... redirect="..."> parameter
 * of the editor form). In that case, the function returns but
 * the browser will redirect the client to another page. This
 * happens after your callback gets called.
 *
 * \sa setTimedoutCallback()
 */
snapwebsites.EditorForm.prototype.formTimedout = function()
{
    var key,
        timeout = /** @type {string} */ (this.formWidget_.attr("timeout")),
        pos;

    // reset the timer if it is still considered active
    // (because this function is public and it may be called by a different
    // mechanism than out internal timer)
    if(!isNaN(this.formTimeoutId_))
    {
        clearTimeout(this.formTimeoutId_);
        this.formTimeoutId_ = NaN;
    }

    // we also do not want to receive auto-reset signals anymore:
    //   1. this function resets the form
    //   2. the form is then not editable anymore (since everything is
    //      disabled)
    //
    if(!isNaN(this.formAutoResetId_))
    {
        clearTimeout(this.formAutoResetId_);
        this.formAutoResetId_ = NaN;
    }

    // disable and restore all the widgets
    //
    // Note: that does not disable buttons that the user may have
    //       created with anchors outside of the editor supported
    //       widgets... this can be done with the callback though
    //
    for(key in this.editorWidgets_)
    {
        if(this.editorWidgets_.hasOwnProperty(key))
        {
            this.editorWidgets_[key].disable();
            this.editorWidgets_[key].restoreValue();
        }
    }

    // make sure the form is marked as unmodified
    // (since we just restored it all!)
    this.wasModified(true);

    // then, if we have a user callback, call it
    if(this.formTimedoutCallback_)
    {
        this.formTimedoutCallback_(this);
    }

    // some forms required that the system redirect the user somewhere
    // else (so the already saved data really gets hidden!)
    //
    pos = timeout.indexOf(",");
    if(pos > 0)
    {
        // we assume that the redirect is always to another page
        location.href = timeout.substr(pos + 1);
    }
};


/** \brief Set the timed out callback for this form.
 *
 * This function is used to setup the timed out callback function.
 * Function which is called once whenever the form session times out.
 * It gives you a chance to act on your form in an additional way
 * since the default is just disabling and resetting the form widgets.
 *
 * This is particularly useful if you use buttons that are direct
 * anchors or if you want to close the popup in which you presented
 * the form to the end user.
 *
 * You may remove the current callback by setting this parameter to
 * \c null.
 *
 * If it is necessary to trigger more than one event, your function
 * is in charge of generating the necessary events.
 *
 * \note
 * Your function may also want to redirect the user. But note that
 * the form is still in memory and accessible via a tool such as
 * FireBug. So you could use your callback to completely delete
 * the form from your HTML tree. We may want to offer such a
 * function here, at some point.
 *
 * @param {function(snapwebsites.EditorForm)} f  The function to be called.
 *
 * \sa formTimedout()
 */
snapwebsites.EditorForm.prototype.setTimedoutCallback = function(f)
{
    this.formTimedoutCallback_ = f;
};


/** \brief Reset the timer to auto-reset this form.
 *
 * This function resets the timer used to know when to auto-reset
 * this form. The auto-reset may not be setup in which case nothing
 * happens.
 *
 * If the \p force flag is set to true, the timer is set. When
 * set to false, it gets reset only if it was already set.
 *
 * \note
 * If the form has no attribute named auto-reset with a value large
 * than zero, then no auto-reset timer is setup by this function.
 *
 * @param {boolean} force  Whether the timer should be set if possible.
 */
snapwebsites.EditorForm.prototype.resetAutoReset = function(force)
{
    var minutes,
        that = this;

    if(!isNaN(this.formAutoResetId_))
    {
        clearTimeout(this.formAutoResetId_);
        this.formAutoResetId_ = NaN;
        force = true;
    }

    if(force)
    {
        // check whether we have an attribute named auto-reset, if so, then
        // setup the auto-reset feature of this form (by default we assume
        // that the form does not need to be auto-resetted)
        minutes = parseFloat(this.formWidget_.attr("auto-reset"));
        if(minutes > 0.0)
        {
            this.formAutoResetId_ = setTimeout(function()
                {
                    that.formAutoReset();
                },
                minutes * 60000); // convert minutes to ms
        }
    }
};


/** \brief The form auto-reset timer just timed out.
 *
 * This function is called whenever the form auto-reset timer
 * times out. The function makes sure to clear the timer
 * (in case it gets called before the auto-reset happens)
 * and then clears any new content the user added.
 *
 * This function does not disable the widgets since these
 * can still be used and submitted, if only the user can
 * enter the data quickly enough and submit it as soon
 * as one is done...
 *
 * \note
 * This function does not setup another auto-reset timer
 * since it would not be useful until the end user starts
 * typing again. That's done then from the changed event.
 */
snapwebsites.EditorForm.prototype.formAutoReset = function()
{
    var key;

    // reset the auto-reset timer
    //
    // note that this function is public and it may be called by a different
    // mechanism than our internal timer
    //
    // also, the auto-reset may be required over and over again, not just
    // once (i.e. the timer does not get reset properly when you start
    // typing again unless it still exists)
    //
    this.resetAutoReset(false);

    // restore the value of each widget; note that in most cases
    // only forms that come out empty should use this feature
    // because using such a feature on a widget that was edited
    // earlier is not useful (i.e. the content is not going to
    // be secret...)
    //
    for(key in this.editorWidgets_)
    {
        if(this.editorWidgets_.hasOwnProperty(key))
        {
            this.editorWidgets_[key].restoreValue();
        }
    }

    // make sure the form is marked as unmodified
    // (since we just restored it all!)
    this.wasModified(true);

    // then, if we have a user callback, call it
    if(this.formAutoResetCallback_)
    {
        this.formAutoResetCallback_(this);
    }
};


/** \brief Set the auto-reset callback for this form.
 *
 * This function is used to setup the auto-reset callback function.
 * This function is called whenever the form auto-reset timer
 * times out. It gives you a chance to act on your form in a different
 * way than the default which is to reset the content of the form widgets.
 *
 * This is particularly useful if you have additional work that needs
 * to happen when such a time out occurs.
 *
 * You may remove the current callback by setting this parameter to
 * null.
 *
 * If it is necessary to trigger more than on event, it is your function
 * responsibility to generate the necessary events.
 *
 * @param {function(snapwebsites.EditorForm)} f  The function to be called.
 */
snapwebsites.EditorForm.prototype.setAutoResetCallback = function(f)
{
    this.formAutoResetCallback_ = f;
};


/** \brief Check whether all types were registered and if so initialize everything.
 *
 * The function checks whether all the types necessary to initialize
 * the widgets are available. If so, then all the form widgets get
 * initialized.
 *
 * \todo
 * Find a way to detect that a form initialization was never completed
 * because the system is missing a widget type.
 */
snapwebsites.EditorForm.prototype.newTypeRegistered = function()
{
    var that = this,
        key;

    if(this.widgetInitialized_)
    {
        return;
    }

    // check whether all the types are available
    for(key in this.usedTypes_)
    {
        if(this.usedTypes_.hasOwnProperty(key)
        && this.usedTypes_[key])
        {
            if(!this.editorBase_.hasWidgetType(key))
            {
                // some types are missing, we're not ready
                // (this happens to forms using external widgets)
                return;
            }

            // this one was defined
            this.usedTypes_[key] = false;
        }
    }

    // if we reach here, all the types are available so we can
    // properly initialize the form now
    this.widgets_.each(function(idx, w)
        {
            var widget_content = jQuery(w),
                name = widget_content.attr("field_name");

            that.editorWidgets_[name] = new snapwebsites.EditorWidget(that.editorBase_, that, widget_content);
        }
    ).each(function(idx, w)
        {
            var widget_content = jQuery(w),
                name = widget_content.attr("field_name"),
                widget = that.editorWidgets_[name];

            widget.getWidgetType().initializeWidget(widget);

            // TODO: add support for:
            //    1. the focus=... query string
            //    2. the first widget with an error
            //    3. then check for .auto-focus
            //
            // auto-focus the widget if so required
            if(widget_content.is(".auto-focus"))
            {
                widget.focus();
            }
        }
    ).each(function(idx, w)
        {
            var widget_content = jQuery(w),
                name = widget_content.attr("field_name"),
                widget = that.editorWidgets_[name];

            // reset the originalData_ & originalValue_ fields
            // TBD: we may want (need) to move this in another loop instead
            widget.discard();
        });

    this.widgetInitialized_ = true;

    // now that all the widgets were initialized we can ready the
    // rest of the form
    this.readyForm_();

    // composite widgets may change their children widgets in a way that
    // marks the editor as modified, clear the flag here so we do not get
    // a spurious popup
    this.modified_ = false;
};


/** \brief This function checks whether the form is to be saved.
 *
 * You may mark a form as a "no-save" form. In that case, the main form
 * \<div\> tag gets a class named "no-save". This function checks for
 * that class.
 *
 * If the class "no-save" is defined on the main form \<div\> tag then
 * this function returns false, otherwise it returns true.
 *
 * This indicates whether the form is expected to be saved. In most cases,
 * a search form or a locator form would be marked as a "no-save" form
 * because entering data and click on a button or another should not
 * generate a save request.
 *
 * \note
 * This function is called on the unload event. If it returns false,
 * then that form is ignored in the test on whether it was modified.
 *
 * @return {boolean} true if the form is to be saved.
 */
snapwebsites.EditorForm.prototype.toBeSaved = function()
{
    return !this.getFormWidget().hasClass("no-save");
};


/** \brief This function checks whether the form was modified.
 *
 * This function checks whether the form was modified. First it
 * checks whether the form is to be saved. If not then the function
 * just returns false.
 *
 * @param {boolean} recheck  Whether to check each widget for modification
 *                           instead of just the modified flags.
 *
 * @return {boolean} true if the form was modified.
 */
snapwebsites.EditorForm.prototype.wasModified = function(recheck)
{
    var key;                            // loop index

    // if already marked as modified and not rechecking, just return true
    //
    if(!recheck && this.modified_)
    {
        return true;
    }

    // check each widget, although we stop on the first one that tells us
    // that it was modified if such exists
    //
    for(key in this.editorWidgets_)
    {
        if(this.editorWidgets_.hasOwnProperty(key)
        && this.editorWidgets_[key].wasModified(recheck))
        {
            this.modified_ = true;
            return true;
        }
    }

    // well, nothing was modified, make sure to mark the form like so
    //
    this.modified_ = false;

    // if the save dialog was opened, make sure to close it now
    //
    this.getSaveDialog().close();

    return false;
};


/** \brief Callback called just before a popup gets closed.
 *
 * This callback gives a chance to the user the save before closing so
 * as to not lose his changes. The editor will also mark the form as
 * saved so if the user clicked cancel it will still not ask the
 * client whether to save when going to another page.
 *
 * The close function can also be Canceled with this feature.
 *
 * @param {Object} popupObject  The popup being closed.
 */
snapwebsites.EditorForm.beforeHide = function(popupObject) // static
{
    var popup = /** @type {snapwebsites.Popup.PopupData} */ (popupObject),
        iframe = popup.widget.find("iframe")[0],
        iframe_window = iframe.contentWindow ? iframe.contentWindow : iframe.contentDocument.defaultView,
        iframe_editor,
        editor_form,
        early_close;

    if(!iframe_window || !iframe_window.snapwebsites)
    {
        // if there is no iframe window then we do not have an editor form
        // so we cannot use the editor early close, just do an immediate
        // close instead as follow:
        popup.hideNow();
        return;
    }

    iframe_editor = iframe_window.snapwebsites.EditorInstance;
    editor_form = iframe_editor.getFormByName(popup.editorFormName);
    if(!editor_form)
    {
        // we may have an iframe window, but still no editor form (i.e.
        // a simple page with links...) so we just want to close the
        // window immediately in this case
        popup.hideNow();
        return;
    }

    early_close = {
        save_mode: snapwebsites.EditorFormBase.SAVE_MODE_SAVE,
        id: "confirm-popup-closure",
        title: "Confirm Closure",
        message: "You made changes to this form, are you sure you just want to close it?",
        buttons: [
            // displayed right to left!
            { name: 'discard', label: 'Yes' },
            { name: 'cancel', label: 'No' }
        ],
        top: 10,
        height: 150,
        callback: function(name)
            {
                if(name === 'discard' || name === 'no-save')
                {
                    popup.hideNow();
                }
            }
    };
    editor_form.earlyClose(early_close);
};


/** \brief Close this form early.
 *
 * This function handles the case when a form is to be closed before the
 * window as a whole is to be closed. You can choose whether the form
 * is to react with a confirmation dialog or not.
 *
 * This is particularly useful in the beforeClose() callback of a popup
 * when that popup is used with a form.
 *
 * The 'message' parameter defines a message to show to the client in case
 * there were modifications. If the message is undefined, then the system
 * just saves the changes and returns.
 *
 * If earlyClose() is to automatically call saveData() ('auto_save' or one
 * of the buttons named 'save' gets clicked,) then the save_mode is used
 * to call saveData(). In most cases it should be set to SAVE_MODE_PUBLISH
 * or SAVE_MODE_SAVE.
 *
 * @param {{save_mode: string,
 *          title: string,
 *          message: string,
 *          buttons: Object.<{name: string, label: string}>,
 *          callback: function(string)}} early_close  The message object with
 *    the message itself, buttons, and a callback function.
 */
snapwebsites.EditorForm.prototype.earlyClose = function(early_close)
{
    var org_callback = early_close.callback,
        that = this;

    if(this.toBeSaved()
    && this.wasModified(true))
    {
        if(!early_close.message)
        {
            this.saveData(early_close.save_mode);
        }
        else
        {
            // our specialized callback for the message box
            early_close.callback = function(name)
                {
                    var key;

                    if(name == "save")
                    {
                        if(early_close.save)
                        {
                            early_close.save(that);
                        }
                        else
                        {
                            that.saveData(early_close.save_mode);
                        }
                    }
                    else if(name == "discard")
                    {
                        // copy the data to make it look like it was saved
                        for(key in that.editorWidgets_)
                        {
                            if(that.editorWidgets_.hasOwnProperty(key))
                            {
                                that.editorWidgets_[key].discard();
                            }
                        }
                        that.getSaveDialog().close();
                    }
                    if(org_callback)
                    {
                        // call the user callback too
                        org_callback(name);
                    }
                };
            snapwebsites.PopupInstance.messageBox(early_close);
        }
    }
    else
    {
        // nothing needs to be saved
        if(early_close.callback)
        {
            early_close.callback("no-save");
        }
    }
};



/** \brief Snap Editor constructor.
 *
 * \note
 * The Snap! Editor is a singleton and should never be created by you. It
 * gets initialized automatically when this editor.js file gets included.
 *
 * \code
 * class Editor extends EditorBase
 * {
 * public:
 *      function Editor() : Editor;
 *      virtual function getToolbar() : EditorToolbar;
 *      virtual function checkModified(editor_widget);
 *      function getActiveEditorForm() : EditorForm;
 *      virtual function getLinkDialog() : EditorLinkDialog;
 *      virtual function registerWidgetType(widget_type: EditorWidgetType);
 *
 * private:
 *      function attachToForms_();
 *      function initUnload_();
 *      function beforeUnload_() : String;
 *      function unloadCanceled_();
 *
 *      toolbar_: EditorToolbar;
 *      editorForms_: Object;
 *      unloadCalled_: boolean;
 *      linkDialog_: EditorLinkDialog;
 * };
 * \endcode
 *
 * @return {snapwebsites.Editor}  The newly created object.
 *
 * @constructor
 * @extends {snapwebsites.EditorBase}
 * @struct
 */
snapwebsites.Editor = function()
{
    snapwebsites.Editor.superClass_.constructor.call(this);

    // attempt to not have URLs and emails transforms to anchors
    document.execCommand("AutoUrlDetect", false, false);

    this.editorForms_ = {};
    this.initUnload_();
    this.attachToForms_();
    this.initUserActivity_();

    return this;
};


/** \brief Editor inherits from EditorBase.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.Editor, snapwebsites.EditorBase);


/** \brief The Editor instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * @type {snapwebsites.Editor}
 */
snapwebsites.EditorInstance = null; // static


/** \brief The toolbar object.
 *
 * This variable represents the toolbar used by the editor.
 *
 * Note that this is the toolbar object, not the DOM. The DOM is
 * defined within the toolbar object and is considered private.
 *
 * @type {snapwebsites.EditorToolbar}
 * @private
 */
snapwebsites.Editor.prototype.toolbar_ = null;


/** \brief List of EditorForm objects.
 *
 * This variable member holds the map of EditorForm objects indexed by
 * their name.
 *
 * \todo
 * We need to ensure that all the forms have a unique name before
 * the JavaScript is used.
 *
 * @type {Object.<snapwebsites.EditorForm>}
 * @private
 */
snapwebsites.Editor.prototype.editorForms_; // = {}; -- initialized in the constructor to avoid problems


/** \brief Whether unload is being processed.
 *
 * There is a "bug" in Firefox and derivative browsers (a.k.a. SeaMonkey)
 * where the browser calls the unload function twice. According to the
 * developers, "it is normal". To avoid that "bug" we use this flag and
 * a timer. If this flag is true, we avoid showing a second prompt to the
 * users.
 *
 * @type {!boolean}
 * @private
 */
snapwebsites.Editor.prototype.unloadCalled_ = false;


/** \brief The link dialog.
 *
 * The get_link_dialog() function creates this link dialog the first
 * time it is called.
 *
 * @type {snapwebsites.EditorLinkDialog}
 * @private
 */
snapwebsites.Editor.prototype.linkDialog_ = null;


/** \brief Attach to the EditorForms defined in the DOM.
 *
 * This function attaches the editor to the existing editor forms
 * as defined in the DOM. Editor forms are detected by the fact
 * that a \<div\> tag has class ".editor-form".
 *
 * The function is expected to be called only once.
 *
 * @private
 */
snapwebsites.Editor.prototype.attachToForms_ = function()
{
    var that = this;

    // retrieve the list of forms using their sessions
    jQuery(".editor-form")
        .each(function(){
            // TBD: We may be able to drop the session map.
            var that_element = jQuery(this),
                session = snapwebsites.castToString(that_element.attr("session"), "editor form session attribute"),
                name = snapwebsites.castToString(that_element.attr("form_name"), "editor form name attribute");

            that.editorForms_[name] = new snapwebsites.EditorForm(that, that_element, name, session);
        });
};


/** \brief Attach to the EditorForms defined in the DOM.
 *
 * This function attaches the editor to the existing editor forms
 * as defined in the DOM. Editor forms are detected by the fact
 * that a \<div\> tag has class ".editor-form".
 *
 * The function is expected to be called only once.
 *
 * @private
 */
snapwebsites.Editor.prototype.initUserActivity_ = function()
{
    var that = this;

    jQuery("body")
        .on("keyup keydown cut paste input textinput", function()
            {
                that.resetFormsAutoReset();
            });
};


/** \brief Resets the timeout of all the forms managed by the editor.
 *
 * This function may be called to extend the timers of all the forms
 * manager by the editor.
 *
 * The forms make use of the auto-reset amount of time to setup a timer.
 * If the timeout is reached, the form gets disabled because the
 * corresponding session will be void on the server side and thus sending
 * that form will always fail. By resetting the forms we extend their
 * use further.
 *
 * \todo
 * This could be really slow for someone typing really fast. We probably
 * want to limit the number of times we reset the form timers, especially
 * because the increment is 1 minute.
 *
 * \todo
 * This function resets all the forms. I wonder whether only the current
 * form should be reset?
 */
snapwebsites.Editor.prototype.resetFormsAutoReset = function()
{
    var key = null;

    for(key in this.editorForms_)
    {
        if(this.editorForms_.hasOwnProperty(key))
        {
            this.editorForms_[key].resetAutoReset(false);
        }
    }
};


/** \brief Retrieve a reference to one of the forms by name.
 *
 * This function takes the name of a form and returns a corresponding
 * reference to the EditorForm object.
 *
 * If the form does not exist, null is returned.
 *
 * @param {string} form_name  The name of the form.
 *
 * @return {snapwebsites.EditorForm}  The corresponding EditorForm object.
 */
snapwebsites.Editor.prototype.getFormByName = function(form_name)
{
    return this.editorForms_[form_name];
};


/** \brief Capture the unload event.
 *
 * This function adds the necessary code to handle the unload event.
 * This is used to make sure that users will save their data before
 * they actually close their browser window or tab.
 *
 * @private
 */
snapwebsites.Editor.prototype.initUnload_ = function()
{
    var that = this;
    jQuery(window)
        .bind("beforeunload", function()
        {
            return that.beforeUnload_();
        });
};


/** \brief Handle the beforeunload event.
 *
 * This function is called whenever the user is about to close their
 * browser window or tab. The function checks whether anything needs
 * to be saved. If so, then make sure to ask the user whether he
 * wants to save his changes before closing the window.
 *
 * @return {null|string|undefined}  A message saying that something
 *          was not saved if it applies, null otherwise.
 *
 * @private
 */
snapwebsites.Editor.prototype.beforeUnload_ = function()
{
    var key,                // loop index
        that = this;        // this pointer in the closure function

    if(!this.unloadCalled_)
    {
        for(key in this.editorForms_)
        {
            if(this.editorForms_.hasOwnProperty(key))
            {
                if(this.editorForms_[key].toBeSaved()
                && this.editorForms_[key].wasModified(true))
                {
                    // add this flag and timeout to avoid a double
                    // "are you sure?" under Firefox browsers
                    this.unloadCalled_ = true;

                    // we create a function in a for() loop because
                    // right after that statement we return
                    setTimeout(function(){
                            that.unloadCanceled_();
                        }, 20);

                    // TODO: translation
                    //       (although it doesn't show up in FireFox based
                    //       browsers many others do show this message)
                    return "You made changes to this page! Click Cancel or Stay on Page to avoid closing the window and Save your changes first.";
                }
            }
        }
    }

    return;
};


/** \brief The unload event was aborted.
 *
 * This function is called whenever the confirm dialog of a closing
 * window is closed.
 *
 * AFter one second, the function triggers an event called
 * unload_maybe_canceled on all the editor form widgets. Something
 * like the following can be used to catch the event:
 *
 * \code
 *      editor = snapwebsites.EditorInstance;
 *      selector = editor.getFormByName("my_form");
 *      selector.getFormWidget()
 *          .bind("unload_maybe_canceled", function(e)
 *              {
 *                  ...
 *              });
 * \endcode
 *
 * The event says "maybe" become in many cases the event will be triggered
 * even if the window is about to be unloaded. There is, unfortunately,
 * no good way, other than waiting \em enough time to know whether the
 * window is going to be closed or not. It is still quite useful to know
 * that the confirmation popup was closed in case you darken the screen
 * because you still will want to undarken the screen, just in case.
 *
 * \warning
 * Note that this function triggers the event on all the editor forms.
 *
 * @private
 */
snapwebsites.Editor.prototype.unloadCanceled_ = function()
{
    this.unloadCalled_ = false;

    setTimeout(function()
        {
            var e = jQuery.Event("unload_maybe_canceled", {});

            jQuery(".editor-form").trigger(e);
        },
        1000);
};


/** \brief Retrieve the toolbar object.
 *
 * This function returns a reference to the toolbar object.
 *
 * @return {snapwebsites.EditorToolbar} The toolbar object.
 * @override
 */
snapwebsites.Editor.prototype.getToolbar = function() // virtual
{
    if(!this.toolbar_)
    {
        this.toolbar_ = new snapwebsites.EditorToolbar(this);
    }
    return this.toolbar_;
};


/** \brief Check whether a field was modified.
 *
 * When something may have changed (a character may have been inserted
 * or deleted, or a text replaced) then you are expected to call this
 * function in order to see whether something was indeed modified.
 *
 * When the process detects that something was modified, it calls the
 * necessary functions to open the Save Dialog.
 *
 * As a side effect it also lets the toolbar know so if it needs to be
 * moved, it happens.
 *
 * \note
 * The editor_widget may be set to null to avoid the widgetchange
 * signal.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget that
 *                               generates the checkModified() call.
 *
 * @override
 */
snapwebsites.Editor.prototype.checkModified = function(editor_widget) // virtual
{
    var active_element,
        active_form = this.getActiveEditorForm(),
        widget_name,
        widget,
        widget_change;

    // checkModified only applies to the active element so make sure
    // there is one when called
    if(active_form)
    {
        // allow the toolbar to adjust itself (move to a new location)
        if(this.toolbar_)
        {
            this.toolbar_.checkPosition();
        }

        // got the form in which this element is defined,
        // now call the wasModified() function on it
        if(active_form.wasModified(false))
        {
            active_form.changed();
        }

        // replace nothingness by "background" value
        active_element = this.getActiveElement();
        widget_name = snapwebsites.castToString(active_element.parent().attr("field_name"), "Editor active element \"field_name\" attribute");
        widget = active_form.getWidgetByName(widget_name);
        widget.checkForBackgroundValue();
    }

    // send an event for each change because the user
    // may want to know even if the value was not actually
    // modified
    if(editor_widget)
    {
        widget = editor_widget.getWidget();
        widget_change = jQuery.Event(
            "widgetchange",
            {
                widget: editor_widget,
                value: editor_widget.getValue()
            });
        widget.trigger(widget_change);
    }
};


/** \brief Retrieve the active editor form.
 *
 * This function gets the editor form that includes the currently
 * active element.
 *
 * @return {snapwebsites.EditorForm}  The active editor form or null.
 */
snapwebsites.Editor.prototype.getActiveEditorForm = function()
{
    var active_element = this.getActiveElement(),
        editor_element,
        editor_form = null,
        name;

    if(active_element)
    {
        editor_element = active_element.parents(".editor-form");
        if(editor_element)
        {
            name = editor_element.attr("form_name");
            editor_form = this.editorForms_[name];
        }
//#ifdef DEBUG
        if(!editor_form)
        {
            // should not happen or it means we whacked the form name
            throw new Error("There is an active element but no corresponding editor form.");
        }
//#endif
    }

    return editor_form;
};


/** \brief Retrieve the link dialog.
 *
 * This function creates an instance of the link dialog and returns it.
 * If the function gets called more than once, then the same reference
 * is returned.
 *
 * The function takes settings defined in an object, the following are
 * the supported names:
 *
 * \li close -- The function called whenever the user clicks the OK button.
 *
 * \param[in] settings  An object defining settings.
 *
 * @return {snapwebsites.EditorLinkDialog} The link dialog reference.
 * @override
 */
snapwebsites.Editor.prototype.getLinkDialog = function()
{
    if(!this.linkDialog_)
    {
        this.linkDialog_ = new snapwebsites.EditorLinkDialog(this);
    }
    return this.linkDialog_;
};


/** \brief Capture the registration of widget types.
 *
 * THis function captures the registration of widget types so it
 * can produce an event on the editor forms which may need additional
 * types whenever they get created to be able to initialize all their
 * widgets.
 *
 * @param {snapwebsites.EditorWidgetType} widget_type  The widget type to register.
 */
snapwebsites.Editor.prototype.registerWidgetType = function(widget_type) // virtual
{
    // first make sure to call the super class function
    snapwebsites.Editor.superClass_.registerWidgetType.call(this, widget_type);

    var key;

    // let all the forms know we have a new type
    //
    // XXX -- this is not really the most efficient, but at this point it
    //        will do
    for(key in this.editorForms_)
    {
        if(this.editorForms_.hasOwnProperty(key))
        {
            this.editorForms_[key].newTypeRegistered();
        }
    }
};



/** \brief Snap EditorWidgetType constructor.
 *
 * The editor works with widgets that are based on this class. Each widget
 * is given a type such as "line-edit".
 *
 * To make it fully dynamic, we define a base class here and let other
 * programmers add new widget types in their own .js files by extending
 * this class.
 *
 * Classes must be registered with the EditorBase class function:
 *
 * \code
 *    snapwebsites.EditorInstance.registerWidgetType(your_widget_type);
 * \endcode
 *
 * This base class already implements a few things that are common to
 * all widgets.
 *
 * \code
 *  class EditorWidgetType extends EditorWidgetTypeBase
 *  {
 *  public:
 *      function EditorWidgetType();
 *      virtual function preInitializeWidget(widget: Object) : Void;
 *      virtual function initializeWidget(widget: Object) : Void;
 *      virtual function setupEditButton(editor_widget: snapwebsites.EditorWidget) : Void;
 *      abstract function droppedImage(e: ProgressEvent, img: Image) : Void;
 *      abstract function droppedAttachment(e: ProgressEvent) : Void;
 *
 *  private:
 *      function droppedImageConvert_(e: ProgressEvent) : Void;
 *      function droppedFiles_(e: ProgressEvent) : Void;
 *      function droppedFileLoaded_(e: ProgressEvent) : Void;
 *  };
 * \endcode
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeBase}
 * @struct
 */
snapwebsites.EditorWidgetType = function()
{
    snapwebsites.EditorWidgetType.superClass_.constructor.call(this);

    // TBD
    // Maybe at some point we'd want to create yet another layer
    // so we can have an auto-register, but I'm not totally sure
    // that would work...
    //snapwebsites.EditorBase.registerWidgetType(this);

    return this;
};


/** \brief EditorWidgetType inherits from EditorWidgetTypeBase.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetType, snapwebsites.EditorWidgetTypeBase);


/** \brief Hold a timer identifier.
 *
 * This variable holds a timer identifier. This is the timer used to
 * check whether a widget got modified between the time the form was
 * loaded and now.
 *
 * The value is NaN as long as the timer is not set.
 *
 * @type {number}
 *
 * @private
 */
snapwebsites.EditorWidgetType.prototype.changeTimerId_ = NaN;


/** \brief Whether we are in a dropzone or not.
 *
 * This variable represents the number of enter/leave while dragging
 * an object over another. The problem we encounter is when the
 * dropzone includes children, in that case, the child is also
 * entered and left and the main object dragenter and dragleave
 * events get triggered if present (probably because the children
 * propagate their event). At this time we use this counter to know
 * whether the user completely left the dropzone or not. We may find
 * a better algorithm later (i.e. drop dragenter and dragleave events
 * from children? test the target?)
 *
 * @type {number}
 *
 * @private
 */
snapwebsites.EditorWidgetType.prototype.dragCounter_ = 0;


/*jslint unparam: true */
/** \brief Initialize a widget of this type.
 *
 * This function is called from the EditorWidget constructor. The core
 * variables will all be initialized when called, but there may be other
 * parts that are not. This function should be used to further some
 * initialization of variable members that does not happen in core.
 *
 * \warning
 * This function is called right after a widget was created and all the
 * widgets may not yet be created when called. In most cases, you want
 * to overload the initializeWidget() function instead.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetType.prototype.preInitializeWidget = function(widget) // virtual
{
};
/*jslint unparam: false */


/** \brief Initialize a widget of this type.
 *
 * Setup various events to handle changes to the widget content and
 * focus.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetType.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent();

    this.setupEditButton(editor_widget); // allow overrides to an empty function

    // widget was possibly modified, so make sure we stay on top
    //
    // Note: "textinput" was used by older versions of Safari
    //
    c.on("keyup cut copy paste input textinput", function()
        {
            // use a timeout so we execute checkModified()
            // AFTER the changes were made... otherwise we are
            // way too early; also make sure we do it just once
            if(isNaN(that.changeTimerId_))
            {
                that.changeTimerId_ = setTimeout(function()
                    {
                        that.changeTimerId_ = NaN;
                        editor_widget.getEditorBase().checkModified(editor_widget);
                    }, 0);
            }
        });

    // widget gets the focus, make it the active widget
    c.focus(function()
        {
            var fixed_header_height;

            w.addClass("focused");

            editor_widget.getEditorBase().setActiveElement(c);

            fixed_header_height = editor_widget.getEditorBase().getFixedHeaderHeight(c);
            if(fixed_header_height > 0
            && c.offset().top - jQuery(window).scrollTop() < fixed_header_height)
            {
                jQuery('html, body').animate(
                    {
                        scrollTop: c.offset().top - fixed_header_height
                    },
                    150);
            }

            if(!jQuery(this).is(".no-toolbar")
            && !jQuery(this).parent().is(".read-only"))
            {
                if(editor_widget.getEditorForm().getToolbarAutoVisible())
                {
                    editor_widget.getEditorBase().getToolbar().toggleToolbar(true);
                }
            }
        });

    // widget loses the focus, lose the toolbar in a few ms...
    //
    // (note: the activeElement_ parameter is NOT reset and this is important
    //        because the toolbar may use it to restore it once the command
    //        is processed.)
    //
    c.blur(function()
        {
            w.removeClass("focused");

            // do not blur the toolbar immediately because if the user just
            // clicked on it, it would break it
            //
            editor_widget.getEditorBase().getToolbar().startToolbarHide();
        });

    // a key was pressed in the focused widget
    c.keydown(function(e)
        {
//console.log("ctrl "+(e.shiftKey?"+ shift ":"")+"= "+e.which+", idx = "+(snapwebsites.EditorInstance.keys_[e.which + (e.shiftKey ? 0x10000 : 0)]));
            if(editor_widget.getEditorBase().getToolbar().keydown(e))
            {
                // toolbar used that key stroke
                e.preventDefault();
                e.stopPropagation();
            }
        });

    // only setup the drag & drop functionality on which that have the
    // class defined
    //
    // TODO: change support for widget that could dynamically add/remove
    //       the drop class along the way?
    //
    if(w.hasClass("drop"))
    {
        // the user just moved over a widget while dragging something
        w.on("dragenter", function(e)
            {
                e.preventDefault();
                e.stopPropagation();

                // allows your CSS to change some things when areas are
                // being dragged over
                if(that.dragCounter_ == 0)
                {
                    w.addClass("dragging-over");
                }
                ++that.dragCounter_;
            });

        // the user is dragging something over a widget
        w.on("dragover", function(e)
            {
                // TBD this is said to make things work better in some browsers...
                e.preventDefault();
                e.stopPropagation();
            });

        // the user just moved out a widget while dragging something
        w.on("dragleave", function(e)
            {
                e.preventDefault();
                e.stopPropagation();

                // remove the class when the mouse leaves
                if(that.dragCounter_ > 0)
                {
                    --that.dragCounter_;
                    if(that.dragCounter_ == 0)
                    {
                        w.removeClass("dragging-over");
                    }
                }
            });

        // the user actually dropped a file on this widget
        //
        // Note: we handle the drop at this level, other widget types
        //       should only override the the droppedImage()
        //       and droppedAttachment() functions instead
        //
        w.on("drop", function(e)
            {
                var i,                      // loop index
                    r,                      // file reader object
                    accept_images,          // boolean, true if element accepts images
                    accept_files,           // boolean, true if element accepts attachments
                    file_loaded;            // finalizing function

                //
                // TODO:
                // At this point this code breaks the normal behavior that
                // properly places the image where the user wants it; I'm
                // not too sure how we can follow up on the "go ahead and
                // do a normal drop instead" without propagating the event,
                // but I will just ask on StackOverflow for now...
                //
                // http://stackoverflow.com/questions/22318243/how-to-apply-the-default-image-drop-behavior-after-testing-that-image-is-valid
                //
                // (abandonned questions get auto-deleted and this is one
                // of them...)
                //
                // That said, I did not get any answer but thinking about
                // it, it seems pretty easy to me: the answer is to use
                // the jQuery().trigger() command which processes the event
                // as if nothing had happened. Then we just need to ignore
                // that event if it calls this "drop" event handler again.
                //

                // remove the dragging-over class on a drop because we
                // do not always get the dragleave event in that case
                //
                w.removeClass("dragging-over");
                that.dragCounter_ = 0;

                // always prevent the default dropping mechanism
                // we handle the file manually all the way
                //
                e.preventDefault();
                e.stopPropagation();

                // anything transferred on widget that accepts files?
                //
                if(e.originalEvent.dataTransfer
                && e.originalEvent.dataTransfer.files.length)
                {
                    that.droppedFiles_(editor_widget, e.originalEvent.dataTransfer.files, false);
                }

                return false;
            });
    }
};


// noempty: false -- not support in current version of jslint
/*jslint unparam: true */
/** \brief Add an "Edit" button to this widget.
 *
 * In general, forms will make use of the "immediate" class which
 * means that no "Edit" button is required. However, a standard page
 * will look pretty much normal except for that little popup that
 * appears when you hover an editable area. Clicking on that "Edit"
 * button in the popup enables the editing of the area.
 *
 * The main reason for doing this is because other functionalities
 * break when editing an area.
 *
 * Widgets that don't offer a "contenteditable" area (i.e.
 * checkmarks, radios, etc.) override this functions with a do
 * nothing function.
 *
 * \note
 * Although we generally expect the getEditButton() to be overriden, this
 * function can also be overridden. This gives you the ability to, for
 * example, generate multiple edit field (i.e. short, normal, and long
 * titles when editing the title of a page.)
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget being
 *                                                   initialized.
 */
snapwebsites.EditorWidgetType.prototype.setupEditButton = function(editor_widget) // virtual
{
};
/*jslint unparam: false */ // noempty: true -- not support in current version


/** \brief Check all the files attached to an element.
 *
 * This function checks all the files that were either dragged and
 * dropped on a DIV or selected with a Browse button (\<input type="file">).
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget that received
 *                                                   the file.
 * @param {FileList} files  An array of files as received by a Drag & Drop
 *                          or an input file element.
 * @param {boolean} display_image  Whether to display the image in the element
 *                                 ('true' for the input file element only.)
 *
 * @return {boolean}  true if at least one file was added.
 *
 * @private
 */
snapwebsites.EditorWidgetType.prototype.droppedFiles_ = function(editor_widget, files, display_image)
{
    var that = this,
        i,
        r,
        file_loaded,
        accept_images,
        accept_files,
        c = editor_widget.getWidgetContent();

    // make sure the element accepts something
    accept_images = c.hasClass("image");
    accept_files = c.hasClass("attachment");
    if(!accept_images && !accept_files)
    {
        return false;
    }

    // we need 'that' so the function has to be dynamically created
    // create it once and re-use any number of times in the loop below
    //
    file_loaded = function(e)
        {
            that.droppedFileLoaded_(e);
        };

    for(i = 0; i < files.length; ++i)
    {
        // TODO: ameliorate support of "plain" attachments
        //
        // For images we do not really care about that info, for uploads we
        // will use it so I keep that here for now to not have to
        // re-research it...
        //console.log("  filename = [" + files[i].name
        //          + "] + size = " + files[i].size
        //          + " + type = " + files[i].type
        //          + "\n");

        // read the image so we can make sure it is indeed an
        // image and ignore any other type of files
        r = new FileReader();
        r.snapEditorWidget = editor_widget;
        r.snapEditorFile = files[i];
        r.snapEditorIndex = i;
        r.snapEditorAcceptImages = accept_images;
        r.snapEditorAcceptFiles = accept_files;
        r.snapEditorDisplayImage = display_image;
        r.onload = file_loaded;

        //
        // TBD: right now we only check the first few bytes
        //      but we may want to increase that size later
        //      to allow for JPEG that have the width and
        //      height defined (much) further in the stream
        //      (at times at the end!?) TIFF is another format
        //      that most often require accessing data near
        //      the end of the file.
        //
        r.readAsArrayBuffer(r.snapEditorFile.slice(0, 64));
    }

    return i > 0;
};


/** \brief Got the content of a dropped file.
 *
 * This function analyze the dropped file content. If recognized then we
 * proceed with the onimagedrop or onattachmentdrop as required.
 *
 * @param {ProgressEvent} e  The event.
 *
 * @private
 * @final
 */
snapwebsites.EditorWidgetType.prototype.droppedFileLoaded_ = function(e)
{
    var that = this,
        r,
        a,
        blob;

    e.target.snapEditorMIME = snapwebsites.OutputInstance.bufferToMIME(e.target.result);
    if(e.target.snapEditorAcceptImages && e.target.snapEditorMIME.substr(0, 6) === "image/")
    {
        // Dropped an Image managed as such

        // It is an image, now convert the data to URI encoding
        // (i.e. base64 encoding) before saving the result in the
        // target element
        //
        r = new FileReader();
        r.snapEditorWidget = e.target.snapEditorWidget;
        r.snapEditorFile = e.target.snapEditorFile;
        r.snapEditorIndex = e.target.snapEditorIndex;
        r.snapEditorAcceptImages = e.target.snapEditorAcceptImages;
        r.snapEditorAcceptFiles = e.target.snapEditorAcceptFiles;
        r.snapEditorMIME = e.target.snapEditorMIME;
        r.onload = function(e)
            {
                that.droppedImageConvert_(e);
            };

        a = [];
        a.push(e.target.snapEditorFile);
        blob = new Blob(a, { type: e.target.snapEditorMIME });
        r.readAsDataURL(blob);
    }
    else if(e.target.snapEditorAcceptFiles && e.target.snapEditorMIME)
    {
        // Dropped a file managed as an attachment
        //
        this.droppedAttachment(e);
    }
    else
    {
        // generate an error
        //
        snapwebsites.OutputInstance.displayOneMessage(
                "File Format Not Known",
                "We do not understand the file format of this file. As such we refuse it for security reasons.");
    }
};


/*jslint eqeq: true */
/** \brief Save the resulting image in the target.
 *
 * This function receives the image as data that can readily be stick
 * in the 'src' attribute of an 'img' tag (i.e. data:image/fmt;base64=...).
 * It gets loaded in an Image object so we can verify the image width
 * and height before it gets presented to the end user.
 *
 * @param {ProgressEvent} e  The file information.
 * @private
 * @final
 */
snapwebsites.EditorWidgetType.prototype.droppedImageConvert_ = function(e)
{
    var that = this,
        img = new Image();

    // The image parameters (width/height) are only available after the
    // onload() event kicks in
    img.onload = function()
        {
            // keep this function here because it is a full closure (it
            // uses 'img', 'that', and even 'e')

            var sizes,
                limit_width = 0,
                limit_height = 0,
                w,
                h,
                nw,
                nh,
                max_sizes;

            // make sure we do it just once
            img.onload = null;

            w = img.width;
            h = img.height;

            if(e.target.snapEditorWidget.getWidgetContent().attr("min-sizes"))
            {
                sizes = e.target.snapEditorWidget.getWidgetContent().attr("min-sizes").split("x");
                if(w < sizes[0] || h < sizes[1])
                {
                    // image too small...
                    // TODO: translation
                    snapwebsites.OutputInstance.displayOneMessage(
                            "Image Too Small",
                            "This image is too small. Minimum required is "
                                + e.target.snapEditorWidget.getWidgetContent().attr("min-sizes")
                                + ". Please try with a larger image.");
                    return;
                }
            }
            if(e.target.snapEditorWidget.getWidgetContent().attr("max-sizes"))
            {
                sizes = e.target.snapEditorWidget.getWidgetContent().attr("max-sizes").split("x");
                if(w > sizes[0] || h > sizes[1])
                {
                    // image too large...
                    // TODO: translation
                    snapwebsites.OutputInstance.displayOneMessage(
                            "Image Too Large",
                            "This image is too large. Maximum allowed is "
                                + e.target.snapEditorWidget.getWidgetContent().attr("max-sizes")
                                + ". Please try with a smaller image.");
                    return;
                }
            }

            if(e.target.snapEditorWidget.getWidgetContent().attr("resize-sizes"))
            {
                max_sizes = e.target.snapEditorWidget.getWidgetContent().attr("resize-sizes").split("x");
                limit_width = max_sizes[0];
                limit_height = max_sizes[1];
            }

            if(limit_width > 0 && limit_height > 0)
            {
                if(w > limit_width || h > limit_height)
                {
                    // source image is too large
                    nw = Math.round(limit_height / h * w);
                    nh = Math.round(limit_width / w * h);
                    if(nw > limit_width && nh > limit_height)
                    {
                        // TBD can this happen?
                        snapwebsites.OutputInstance.displayOneMessage(
                                "Image Too Large",
                                "somehow we could not adjust the dimentions of this image properly!?");
                    }
                    if(nw > limit_width)
                    {
                        h = nh;
                        w = limit_width;
                    }
                    else
                    {
                        w = nw;
                        h = limit_height;
                    }
                }
                jQuery(img)
                    .css({top: (limit_height - h) / 2, left: (limit_width - w) / 2, position: "relative"});
            }
            jQuery(img)
                .attr("width", w)
                .attr("height", h)
                .attr("filename", e.target.snapEditorFile.name);

            that.droppedImage(e, img);
        };
    img.src = e.target.result;

    // TBD: still a valid test? img.readyState is expected to be a string!
    //
    // a fix for browsers that do not call onload() if the image is
    // already considered loaded by now
    //
    if(img.complete || img.readyState == 4)
    {
        img.onload();
    }
};
/*jslint eqeq: false */


/*jslint unparam: true */
/** \brief Handle an image that was just dropped.
 *
 * This function handles an image as it was just dropped.
 *
 * The default function is abstract (to simulate the abstractness, we throw
 * here.) Only widgets with a type that overloads this function understand
 * images.
 *
 * @param {ProgressEvent} e  The reader data.
 * @param {Image} img  The image te user dropped.
 */
snapwebsites.EditorWidgetType.prototype.droppedImage = function(e, img) // abstract
{
    throw new Error("snapwebsites.EditorWidgetType.prototype.droppedImage() not overridden and thus it cannot handle the dropped image.");
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Handle an image that was just dropped.
 *
 * This function handles an image as it was just dropped.
 *
 * @param {ProgressEvent} e  The event.
 */
snapwebsites.EditorWidgetType.prototype.droppedAttachment = function(e) // abstract
{
    throw new Error("snapwebsites.EditorWidgetType.prototype.droppedAttachment() not overridden and thus it cannot handle the dropped attachment.");
};
/*jslint unparam: false */



/** \brief The constructor of the widget type Content Editable.
 *
 * This intermediate type offers an "Edit" button for the user to enter
 * editing mode on an area which by default won't be editable.
 *
 * Note that widgets can be marked as immediate in order to force the
 * editing capability of the widget and bypass this "Edit" button.
 *
 * \code
 *  class EditorWidgetTypeContentEditable extends EditorWidgetType
 *  {
 *  public:
 *      function EditorWidgetTypeContentEditable();
 *      function setupEditButton(editor_widget: snapwebsites.EditorWidget) : Void;
 *      virtual function getEditButton() : string;
 *  };
 * \endcode
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeContentEditable = function()
{
    snapwebsites.EditorWidgetTypeContentEditable.superClass_.constructor.call(this);

    return this;
};


/** \brief EditorWidgetTypeContentEditable inherits from EditorWidgetType.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeContentEditable, snapwebsites.EditorWidgetType);


/** \brief Initialize an "Edit" button.
 *
 * This function adds an "Edit" button to the specified \p editor_widget.
 * By default most widgets do not get an edit button because they are
 * automatically editable (if not disabled or marked as read-only.)
 *
 * This is especially necessary on editable text areas where links would
 * otherwise not react to clicks since editable areas accept focus instead.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget being
 *                                                   initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeContentEditable.prototype.setupEditButton = function(editor_widget)
{
    var that = this,
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        html,
        edit_button_popup;

    if(w.is(".immediate"))
    {
        // editor is immediately made available
        // (most usual in editor forms)
        c.attr("contenteditable", "true");
        return;
    }

    // get the HTML of the "Edit" button
    html = this.getEditButton();
    if(!html)
    {
        return;
    }

    jQuery(/** @type {string} */ (html)).prependTo(w);

    edit_button_popup = w.children(".editor-edit-button");

    // correct the font size, unfortunately there does not seem to be
    // a good way to do so in CSS without knowing the general document
    // font size...
    //
    edit_button_popup.css("font-size", parseFloat(jQuery("html").css("font-size")));

    // user has to click Edit to activate the editor
    edit_button_popup.children(".activate-editor").click(function(e)
        {
            e.preventDefault();

            // simulate a mouseleave so the edit button form gets hidden
            // then remove the hover events
            w.mouseleave().off("mouseenter mouseleave");

            // request the original data which is about to be edited;
            // this is important if it included any tokens or similar
            // fields
            that.ajaxReason_ = "request_original_data";
            that.ajaxWidget_ = editor_widget;
            if(!that.serverAccess_)
            {
                that.serverAccess_ = new snapwebsites.ServerAccess(that);
            }
            that.serverAccess_.setURI(snapwebsites.castToString(jQuery("link[rel='page-uri']").attr("href"), "casting href of the page-uri link to a string in snapwebsites.EditorWidgetTypeContentEditable.setupEditButton()"));
            that.serverAccess_.setData({
                            _editor_request_original_data: 1,
                            page_language: jQuery("html").attr("lang"),
                            field_name: editor_widget.getName()
                        });
            that.serverAccess_.send(e);
        });

    // this adds the mouseenter and mouseleave events
    w.hover(
        function()  // mouseenter
        {
            edit_button_popup.fadeIn(150);
        },
        function()  // mouseleave
        {
            edit_button_popup.fadeOut(150);
        });
};


/** \brief The HTML representing the "Edit" button.
 *
 * This function returns the default "Edit" button HTML. It can be
 * overridden in order to change the button.
 *
 * \note
 * If the function returns an empty string or false then it is assumed
 * that the object does not want an edit button (i.e. a checkmark object.)
 *
 * @return {!string|!boolean}  A string representing the HTML of the "Edit"
 *                             button or false.
 */
snapwebsites.EditorWidgetTypeContentEditable.prototype.getEditButton = function() // virtual
{
    return "<div class='editor-edit-button'><a class='activate-editor' href='#'>Edit</a></div>";
};


/** \brief Manage request to retrieve field original content.
 *
 * We just sent an AJAX request to retrieve the field original content.
 * This content is to be shown in the widget in place of the final
 * content (i.e. without all the tokens replaced, filters applied, etc.)
 *
 * The function makes sure that the AJAX reason is indeed
 * "request_original_data" and if so, it starts the editing
 * of the widget.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.EditorWidgetTypeContentEditable.prototype.serverAccessSuccess = function(result) // virtual
{
    var editor_widget,
        w,
        c,
        xml_data,
        attachment_path;

    snapwebsites.EditorWidgetTypeContentEditable.superClass_.serverAccessSuccess.call(this, result);

    if(this.ajaxReason_ == "request_original_data")
    {
        editor_widget = this.ajaxWidget_;
        w = editor_widget.getWidget();
        c = editor_widget.getWidgetContent();

        xml_data = jQuery(result.jqxhr.responseXML);
        c.html(snapwebsites.castToString(xml_data.find("data[name='field_data']").text(), "casting field_data to a string"));

        // make the child editable
        // TODO: either select all or at least place the cursor at the
        //       end in some cases...
        c.attr("contenteditable", "true");

        // give the widget focus if not disabled
        editor_widget.focus();
    }
};



/** \brief Editor widget type for Hidden widgets.
 *
 * This widget defines hidden elements in the editor forms. This is
 * an equivalent to the hidden input element of a standard form, although
 * our hidden widgets can include any type of text.
 *
 * \code
 *  class EditorWidgetTypeHidden extends EditorWidgetType
 *  {
 *  public:
 *      virtual function getType() : string;
 *      virtual function initializeWidget(widget: Object) : Void;
 *  };
 * \endcode
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeHidden = function()
{
    snapwebsites.EditorWidgetTypeHidden.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeHidden, snapwebsites.EditorWidgetType);


/** \brief Return "hidden".
 *
 * Return the name of the hidden type.
 *
 * @return {string} The name of the hidden type.
 * @override
 */
snapwebsites.EditorWidgetTypeHidden.prototype.getType = function() // virtual
{
    return "hidden";
};


/** \brief Initialize the widget.
 *
 * This function initializes the hidden widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeHidden.prototype.initializeWidget = function(widget) // virtual
{
    snapwebsites.EditorWidgetTypeHidden.superClass_.initializeWidget.call(this, widget);
};



/** \brief Editor widget type for Text Edit widgets.
 *
 * This widget defines the full text edit in the editor forms. This is
 * an equivalent to the text area of a standard form.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeContentEditable}
 * @struct
 */
snapwebsites.EditorWidgetTypeTextEdit = function()
{
    snapwebsites.EditorWidgetTypeTextEdit.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeTextEdit, snapwebsites.EditorWidgetTypeContentEditable);


/** \brief Return "text-edit".
 *
 * Return the name of the widget type.
 *
 * @return {string} The name of the widget type.
 * @override
 */
snapwebsites.EditorWidgetTypeTextEdit.prototype.getType = function() // virtual
{
    return "text-edit";
};


/** \brief Initialize the widget.
 *
 * This function initializes the text-edit widget. It setups a keydown
 * event to prevent editing if the widget is marked as read-only or
 * disabled.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeTextEdit.prototype.initializeWidget = function(widget) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        c = editor_widget.getWidgetContent();

    snapwebsites.EditorWidgetTypeTextEdit.superClass_.initializeWidget.call(this, widget);

    c.keydown(function(e)
        {
            var w = c.parent();

            // do not prevent the Tab and Enter
            if(e.which == 9 || e.which == 13)
            {
                return;
            }

            if(w.is(".read-only"))
            {
                switch(e.which)
                {
                case 33:    // page up
                case 34:    // page down
                case 35:    // end
                case 36:    // home
                case 37:    // arrow left
                case 38:    // arrow up
                case 39:    // arrow right
                case 40:    // arrow down
                    return;

                case 67:    // Ctlr-C (copy text)
                    if(e.ctrlKey)
                    {
                        return;
                    }
                    break;

                }
                e.preventDefault();
                e.stopPropagation();
                return;
            }

            if(w.is(".disabled"))
            {
                // no typing allowed
                e.preventDefault();
                e.stopPropagation();
            }
        });
};


/** \brief Save a new value in the specified editor widget.
 *
 * This function offers a way for programmers to dynamically change the
 * value of a widget. You should never call the editor widget type
 * function, instead use the setValue() function of the widget you
 * want to change the value of (it will make sure that the modified
 * flag is properly set.)
 *
 * Depending on the type, the value may be a string, a number, of some
 * other type, this is why here it is marked as an object.
 *
 * @param {!Object} widget  The concerned widget.
 * @param {!Object|string|number} value  The value to be saved.
 *
 * @return {boolean}  true if the value gets changed.
 */
snapwebsites.EditorWidgetTypeTextEdit.prototype.setValue = function(widget, value)
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        c = editor_widget.getWidgetContent();

    if(c.html() !== value)
    {
        c.empty();
        c.append(/** @type {string} */ (value));
        return true;
    }

    return false;
};


/** \brief Validate the content of a widget.
 *
 * This function verifies that the text edit has the required minimum
 * length, does not go over the maximum length, etc.
 *
 * @param {Object} widget  The editor widget.
 *
 * @return {boolean}  true if the widget content is currently considered valid.
 */
snapwebsites.EditorWidgetTypeTextEdit.prototype.validate = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        label = "<strong>" + editor_widget.getLabel(true) + "</strong>",
        value = editor_widget.getValue(),
        stripped_value = snapwebsites.stripAllTags(value),
        bound,
        valid = true;

    // too short?
    bound = /** @type {number} */ (c.data("absoluteminlength"));
    if(jQuery.isNumeric(bound)
    && value.length < bound)
    {
        valid = false;
        snapwebsites.OutputInstance.displayOneMessage(
                "Invalid Field",
                "Entry \"" + (stripped_value.length > 64 ? stripped_value.substr(0, 61) + "..." : stripped_value) + "\" too short in " + label + ".",
                "error",
                true);
    }
    else
    {
        bound = /** @type {number} */ (c.data("minlength"));
        if(jQuery.isNumeric(bound)
        && stripped_value.length < bound)
        {
            valid = false;
            snapwebsites.OutputInstance.displayOneMessage(
                    "Invalid Field",
                    "Entry \"" + (stripped_value.length > 64 ? stripped_value.substr(0, 61) + "..." : stripped_value) + "\" is too short for " + label + ".",
                    "error",
                    true);
        }
        else
        {
            bound = /** @type {number} */ (c.data("absolutemaxlength"));
            if(jQuery.isNumeric(bound)
            && value.length > bound)
            {
                valid = false;
                snapwebsites.OutputInstance.displayOneMessage(
                        "Invalid Field",
                        "Entry too long in " + label + ".",
                        "error",
                        true);
            }
            else
            {
                // too long?
                bound = /** @type {number} */ (c.data("maxlength"));
                if(jQuery.isNumeric(bound)
                && stripped_value.length > bound)
                {
                    valid = false;
                    snapwebsites.OutputInstance.displayOneMessage(
                            "Invalid Field",
                            "Entry too long in " + label + ".",
                            "error",
                            true);
                }
            }
        }
    }

    // make sure the "erroneous" class is as expected
    w.toggleClass("erroneous", !valid);

//console.log("widget: \"" + editor_widget.getName() + "\" (" + label + ") value [" + value + "]:" + value.length + "/" + stripped_value.length + " -> " + (valid ? "VALID!!!" : "invalid?!?!"));

    return valid;
};



/** \brief Editor widget type for Text Edit widgets.
 *
 * This widget defines the "line-edit" in the editor forms. This is
 * an equivalent to the input tag of type text of a standard form.
 *
 * @return {!snapwebsites.EditorWidgetTypeLineEdit}
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeTextEdit}
 * @struct
 */
snapwebsites.EditorWidgetTypeLineEdit = function()
{
    snapwebsites.EditorWidgetTypeLineEdit.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeLineEdit, snapwebsites.EditorWidgetTypeTextEdit);


/** \brief Return "line-edit".
 *
 * Return the name of the widget type.
 *
 * @return {string} The name of the widget type.
 * @override
 */
snapwebsites.EditorWidgetTypeLineEdit.prototype.getType = function()
{
    return "line-edit";
};


/** \brief Initialize the widget.
 *
 * This function initializes the line-edit widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeLineEdit.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        c = editor_widget.getWidgetContent();

    snapwebsites.EditorWidgetTypeLineEdit.superClass_.initializeWidget.call(this, widget);

    c.keydown(function(e)
        {
            var editor_form;

            if(e.which === 13 && that.acceptReturnAsDefaultButton())
            {
                // prevent enter default behavior
                //
                e.preventDefault();

                // if editor has a default button, click it
                //
                editor_form = editor_widget.getEditorForm();
                editor_form.getDefaultButton().click();
            }
        });

    c.on("paste", function()
        {
            // after a paste we may have HTML that we would need to remove
            setTimeout(function()
                {
                    that.verifyValue_(editor_widget);
                }, 0);
        });
};


/** \brief Check whether Return is allowed to auto-send the form.
 *
 * This is a virtual function that can be used to prevent the default
 * behavior of the Return key which auto-clicks the default button
 * if this function returns true (which is the default.)
 *
 * @return {boolean}  true if the Return is allowed to auto-click the
 *                    default button to auto-send the form to the server.
 */
snapwebsites.EditorWidgetTypeLineEdit.prototype.acceptReturnAsDefaultButton = function()
{
    return true;
};


/** \brief Setup the verification value.
 *
 * This function verifies that the widget value does not include
 * HTML data. You may ask to keep basic HTML, but most will be
 * removed (i.e. blocks, br, etc.)
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget that was just modified.
 *
 * @private
 */
snapwebsites.EditorWidgetTypeLineEdit.prototype.verifyValue_ = function(editor_widget)
{
    var w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        keep_html = w.is(".keep-html"),
        selection;

    if(keep_html)
    {
        // TODO: parse the DOM and remove anything that we do not like
        //       at the very list we have to remove <br/> and any block
        //       tags (<p>, <div>, etc.) although we probably want to
        //       list tags we accept to keep instead (i.e. <b>, <strong>,
        //       <em>, <i>, etc.)
    }
    else
    {
        // remove ALL tags if any
        if(c.html() != c.text())
        {
            // TODO: fix selection problems
            // the select is completely lost afterward... we would
            // need to transform all the tags to text to calculate
            // the position as if no tags were present, then "restore"
            // accordingly (i.e. set the position appropriately and
            // not using a previously saved selection object.)
            selection = snapwebsites.EditorSelection.saveSelection();
            c.text(/** @type {string} */ (c.text()));
            snapwebsites.EditorSelection.restoreSelection(selection);
        }
    }
};



/** \brief Editor widget type for Dropdown widgets.
 *
 * This widget defines a dropdown in the editor forms.
 *
 * \note
 * The EditorWidgetTypeDropdown holds a reference to the last dropdown
 * items that was opened so it can close it in the event any other
 * dropdown happened to get opened.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeLineEdit}
 * @struct
 */
snapwebsites.EditorWidgetTypeDropdown = function()
{
    snapwebsites.EditorWidgetTypeDropdown.superClass_.constructor.call(this);

    return this;
};


/** \brief EditorWidgetTypeDropdown inherits from EditorWidgetTypeLineEdit.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeDropdown, snapwebsites.EditorWidgetTypeLineEdit);


/** \brief The currently opened dropdown.
 *
 * This variable member holds the jQuery widget of the currently opened
 * dropdown. It is used to be able to close that widget whenever another
 * one gets opened or the widget loses focus.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.openDropdown_ = null;


/** \brief Whether the currently opened dropdown is a clone.
 *
 * This variable member is true when the openDropdown_ variable is
 * a clone of the dropdown elements. This means it will be deleted
 * after it gets closed.
 *
 * By default, this is false as it is assumed that your dropdown is
 * created in the top-most window.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.clonedDropdown_ = false;


/** \brief Return "dropdown".
 *
 * Return the name of the dropdown type.
 *
 * @return {string} The name of the dropdown type.
 * @override
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.getType = function() // virtual
{
    return "dropdown";
};


/** \brief Initialize the widget.
 *
 * This function initializes the dropdown widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        d = w.children(".dropdown-items");

    snapwebsites.EditorWidgetTypeDropdown.superClass_.initializeWidget.call(this, widget);

    //
    // WARNING: we dynamically assign the "absolute" flag because otherwise
    //          the item doesn't get positioned right at the start (i.e. in
    //          this way we don't have to compute the position.)
    //
    d.css("position", "absolute");

    // Click hides currently opened dropdown if there is one.
    // Then shows the dropdown of the clicked widget (unless that's the one
    // we just closed.)
    w.click(function(e)
        {
            var visible = w.is(".disabled") || that.isDropdownOpen();

            // avoid default browser behavior
            e.preventDefault();
            e.stopPropagation();

            // hide the dropdown AFTER we checked its visibility
            that.hideDropdown();

            if(!visible)
            {
                that.openDropdown(editor_widget);
            }
        });

    d.children(".dropdown-selection")
        .children(".dropdown-item")
        .click(function(e)
            {
                // avoid default browser behavior
                e.preventDefault();
                e.stopPropagation();

                that.itemClicked(editor_widget, jQuery(this));
            });

    c.blur(function()
        {
            that.hideDropdown();
        })
     .keydown(function(e)
        {
            var item;

            switch(e.which)
            {
            case 40: // arrow down
                // avoid default browser behavior
                e.preventDefault();

                if(that.isDropdownOpen())
                {
                    that.selectItem(editor_widget, "down");
                }
                else if(!w.is(".disabled"))
                {
                    that.openDropdown(editor_widget);
                    that.selectItem(editor_widget, "first");
                }
                break;

            case 38: // arrow up
                // avoid default browser behavior
                e.preventDefault();

                if(that.isDropdownOpen())
                {
                    that.selectItem(editor_widget, "up");
                }
                else if(!w.is(".disabled"))
                {
                    that.openDropdown(editor_widget);
                    that.selectItem(editor_widget, "last");
                }
                break;

            case 37: // arrow left
                if(that.isDropdownOpen())
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    that.selectItem(editor_widget, "left");
                }
                break;

            case 39: // arrow right
                if(that.isDropdownOpen())
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    that.selectItem(editor_widget, "right");
                }
                break;

            case 36: // home
                if(that.isDropdownOpen())
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    that.selectItem(editor_widget, "first"); // should be first-item-or-first-column
                }
                break;

            case 35: // end
                if(that.isDropdownOpen())
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    that.selectItem(editor_widget, "last"); // should be last-item-or-last-column
                }
                break;

            case 33: // page up
                if(that.isDropdownOpen())
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    that.selectItem(editor_widget, "top"); // should be first-item-or-first-column
                }
                break;

            case 34: // page down
                if(that.isDropdownOpen())
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    that.selectItem(editor_widget, "bottom"); // should be last-item-or-last-column
                }
                break;

            case 27: // escape (close, no changes)
                if(that.isDropdownOpen())
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    that.hideDropdown();
                }
                break;

            case 13: // return (select)
                if(that.isDropdownOpen())
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    e.stopPropagation();

                    item = that.openDropdown_.children(".dropdown-selection")
                                             .children(".dropdown-item")
                                             .filter(".active");
                    if(item.exists())
                    {
                        that.itemClicked(editor_widget, item);
                    }
                }
                break;

            default:
                // not too sure how we could extend that to UTF-16...
                // right now, A to Z works well
                if(that.isDropdownOpen() && e.which >= 65 && e.which <= 90)
                {
                    that.selectItem(editor_widget, String.fromCharCode(e.which).toLowerCase());
                }
                break;

            }
        });
};


/** \brief Check whether the dropdown is open.
 *
 * This function returns true if the dropdown is currently open.
 *
 * Testing whether a widget is visible (":visible" in CSS/jQuery) is
 * not going to work correctly if the dropdown opens using a cloned
 * version of the dropdown list.
 *
 * @return {boolean}  true if the dropdown is currently opened.
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.isDropdownOpen = function()
{
    return !!this.openDropdown_;
};


/** \brief Open the specified dropdown object.
 *
 * This function opens the dropdown selection window. This selection
 * always appears in the top window so it can, without a problem, extend
 * outside of a popup.
 *
 * he dropdown window will also showing multiple columns if there is
 * not enough room to show all the items in a single column.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget being initialized.
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.openDropdown = function(editor_widget)
{
    var that = this,
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        d = w.children(".dropdown-items"),
        x,
        y,
        z,
        pos = w.position(),
        iframe,
        iframe_pos,
        dropdown_position = "absolute",
        algorithm_instructions = w.data("algorithm"),
        algorithm,
        key,
        width,
        height,
        max_count,
        count,
        screen_width = parseFloat(jQuery(window.top).innerWidth()),
        screen_height = parseFloat(jQuery(window.top).innerHeight()),
        scroll_top = parseFloat(jQuery(window.top).scrollTop()),
        scroll_left = parseFloat(jQuery(window.top).scrollLeft()),
        valid = false,
        new_class,
        equal_pos,
        selected,
        offset,
        column_range,
        list_of_items,
        number_of_items,
        col_widths,
        col_outer_widths,
        total_width,
        tallest_line,
        dropdown_height,
        start_top;

    // is the window an iframe or the top window?
    //
    // test with 'window.' so it works in IE
    this.clonedDropdown_ = window.self != window.top;
    if(this.clonedDropdown_)
    {
        // Just in case, I am keeping the window.name trick here, but it looks
        // like all browsers have the window.frameElement parameter set properly
        //var name = window.name;
        //iframe_pos = window.top.jQuery("#" + name + ".snap-popup .popup-body iframe").offset();

        // TODO: this works because we have only one layer at this time, we
        //       want to look at supporting any number of iframe layers though
        //
        iframe = jQuery(window.frameElement);
        iframe_pos = iframe.offset();
        pos.top += iframe_pos.top - scroll_top;
        pos.left += iframe_pos.left - scroll_left;

        // At this point I do not understand this one, but this works...
        scroll_top = 0;
        scroll_left = 0;

        // if the user is to scroll the page while the dropdown is open
        // from an iframe, we must make sure it has the same positioning
        // otherwise it will scroll/not-scroll unaccordingly
        //
        if(iframe.parents("div.snap-popup").css("position") == "fixed")
        {
            dropdown_position = "fixed";
        }

        // cloning by converting to text and back to a DOM, this works
        // across frames without having to deal with fragments
        //
        this.openDropdown_ = window.top.jQuery("<div class='top-window dropdown-items zordered'>"
                                + d.html() + "</div>").appendTo("body");

        this.openDropdown_
            .children(".dropdown-selection")
            .children(".dropdown-item")
            .click(function(e)
                {
                    // avoid default browser behavior
                    e.preventDefault();
                    e.stopPropagation();

                    that.itemClicked(editor_widget, jQuery(this));
                });
    }
    else
    {
        // the newly visible dropdown
        this.openDropdown_ = d;
    }

    this.openDropdown_.css("position", dropdown_position);

    // setup z-index
    // (reset itself first so we do not just +1 each time)
    this.openDropdown_.css("z-index", 0);
    z = window.top.jQuery(".zordered").maxZIndex() + 1;
    this.openDropdown_.css("z-index", z);

    // position, this makes use of the "dropdown algorithm" that the
    // user can specified when creating the widget
    //
    if(!algorithm_instructions)
    {
        // a default algorithm so something happens...
        //
        algorithm_instructions = "bottom top right bottom-vertical-columns=* top-vertical-columns=* bottom-vertical-columns-with-scrollbar=* top-vertical-columns-with-scrollbar=* scrollbar";
    }
    algorithm_instructions = algorithm_instructions.split(" ");
    algorithm_instructions.push("forced-bottom"); // make sure we always have a default

    this.openDropdown_.fadeIn(150);

    // WARNING: position() gives us the top-left corner including the
    //          top and left margin of the widget. For this reason,
    //          we have to adjust here to get the top-left corner of
    //          the border edge instead:
    pos.top += parseFloat(w.css("margin-top"));
    pos.left += parseFloat(w.css("margin-left"));

    for(key in algorithm_instructions)
    {
        if(algorithm_instructions.hasOwnProperty(key))
        {
            algorithm = algorithm_instructions[key];
//console.log(new Date);
//console.log("check algorithm [" + algorithm + "]");

            // the dropdown information may vary because of this class
            new_class = algorithm;
            if(new_class.substr(0, 7) == "forced-")
            {
                new_class = new_class.substr(7);
            }
            equal_pos = new_class.indexOf("=");
            if(equal_pos > 0)
            {
                new_class = new_class.substr(0, equal_pos);
            }
            new_class = "algorithm-" + new_class;
            this.openDropdown_.addClass(new_class);
            width = this.openDropdown_.outerWidth(false);
            height = this.openDropdown_.outerHeight(false);  // inner height + padding + borders, but not margin nor shadow

            switch(algorithm)
            {
            case "bottom":
                if(pos.top + w.outerHeight(false) + height - scroll_top > screen_height)
                {
                    break;
                }
            case "forced-bottom":
                pos.top += w.outerHeight(false);
                if(pos.left + width > screen_width)
                {
                    // adjust the left position so we see the right side of the dropdown
                    pos.left = screen_width - width;
                    if(pos.left < 0)
                    {
                        pos.left = 0;
                    }
                }
                valid = true;
                break;

            case "top":
                if(pos.top + parseFloat(w.css("border-top-width")) - this.openDropdown_.outerHeight(true) < scroll_top)
                {
                    break;
                }
            case "forced-top":
                pos.top += parseFloat(w.css("border-top-width")) - this.openDropdown_.outerHeight(true);
                valid = true;
                break;

            case "right":
                if(pos.left + w.outerWidth(false) + width - scroll_left > screen_width
                || height > screen_height)
                {
                    break;
                }
            case "forced-right":
                pos.left += w.outerWidth(false);
                pos.top = scroll_top + (screen_height - height) / 2;
                valid = true;
                break;

            case "scrollbar":
                // we assume that if we reach here the size of the dropdown
                // is such that it does not really fit up or down so we
                // display it in fullscreen;
                //
                if(height > screen_height)
                {
                    height = screen_height;
                }
                pos.top = scroll_top;
                if(pos.left + width > screen_width)
                {
                    // adjust the left position so we see the right side of the dropdown
                    pos.left = screen_width - width;
                    if(pos.left < 0)
                    {
                        pos.left = 0;
                    }
                }
                this.openDropdown_
                        //.find("ul")
                        .outerHeight(height)
                        .width(this.openDropdown_.outerWidth(false) + 20);     // +20 to account for the scrollbar width (TODO: calculate exact size)

                // now scroll toward the existing selection if any
                selected = this.openDropdown_.find("li.selected");
                if(selected.exists())
                {
                    // TBD: by setting the overflow to hidden, we will force the scrollTop()
                    // to be recalculated/used as expected (may be required for mobile phones)
                    //this.openDropdown_.find("ul").css("overflow-y", "hidden");
                    this.openDropdown_.scrollTop(0);
                    offset = (height - selected.outerHeight()) / 2;
                    if(offset < 0)
                    {
                        offset = 0;
                    }
                    this.openDropdown_.scrollTop(selected.position().top - offset);
                    //this.openDropdown_.find("ul").css("overflow-y", "");
                }
                valid = true;
                break;

            default:
                // algorithms that include an equal sign are managed here
                algorithm = algorithm.split("=");
                switch(algorithm[0])
                {
                case "bottom-horizontal-columns":
                    if(algorithm[1] == "*")
                    {
                        // try until column count == number of items
                        // or the width fails
                        max_count = 10;
                        count = 2;
                    }
                    else
                    {
                        // exact count (count[-max])
                        column_range = algorithm[1].split("-");
                        count = parseInt(column_range[0], 10);
                        if(column_range.length == 1)
                        {
                            max_count = count;
                        }
                        else
                        {
                            max_count = parseInt(column_range[1], 10);
                        }
                    }
                    // limit to a maximum of 20 attempts
                    if(max_count - count > 20)
                    {
                        max_count = count + 20;
                    }
                    // TODO: if we have a single item, we fail the for()
                    //       loop below; that's probably not 100% correct
                    //       to do so, however...
                    //
                    number_of_items = this.openDropdown_.find("li").length;
                    if(max_count > number_of_items)
                    {
                        max_count = number_of_items;
                    }
                    for(; count <= max_count; ++count)
                    {
                        // to place columns horizontally, we have to use
                        // float on each li and force a width depending on
                        // the column
                        //
                        col_widths = new Array(count);
                        col_outer_widths = new Array(count);
                        this.openDropdown_
                            .find("li")
                            .each(function(idx, element)
                                {
                                    var col = idx % count,
                                        width = jQuery(element).width(),
                                        outer_width = jQuery(element).outerWidth(true);

                                    if(!col_widths[col]
                                    || width > col_widths[col])
                                    {
                                        // largest item in this column
                                        col_widths[col] = width;
                                    }

                                    if(!col_outer_widths[col]
                                    || outer_width > col_outer_widths[col])
                                    {
                                        // largest item in this column
                                        col_outer_widths[col] = outer_width;
                                    }
                                });
                        // get resulting width (necessary to make sure
                        // the float are moved to the next line)
                        total_width = 0;
                        jQuery.each(col_outer_widths, function(name, value)
                            {
                                total_width += value;
                            });
                        this.openDropdown_
                            .find("ul")
                            .width(total_width);
                        this.openDropdown_
                            .find("li")
                            .css("float", "left")
                            .width(function(idx, element)
                                {
                                    var col = idx % count;

                                    return col_widths[col];
                                });

                        width = this.openDropdown_.outerWidth(false);
                        if(width > screen_width)
                        {
                            break;
                        }
                        if(pos.top + w.outerHeight(false) + this.openDropdown_.outerHeight(false) - scroll_top < screen_height)
                        {
                            pos.top += w.outerHeight(false);
                            if(pos.left + width > screen_width)
                            {
                                // adjust the left position so we see the right side of the dropdown
                                pos.left = screen_width - width;
                                if(pos.left < 0)
                                {
                                    pos.left = 0;
                                }
                            }
                            valid = true;
                            break;
                        }
                    }
                    if(!valid)
                    {
                        // remove the column css on failures
                        this.openDropdown_.find("ul").css("width", "");
                        this.openDropdown_.find("li").css("width", "").css("float", "");
                    }
                    break;

                case "top-horizontal-columns":
                    if(algorithm[1] == "*")
                    {
                        // try until column count == number of items
                        // or the width fails
                        max_count = 10;
                        count = 2;
                    }
                    else
                    {
                        // TODO: add a way to offer a range instead of only
                        //       a fixed value or any number of columns
                        //
                        // exact count
                        count = parseInt(algorithm[1], 10);
                        max_count = count;
                    }
                    // TODO: if we have a single item, we fail the for()
                    //       loop below; that's probably not 100% correct
                    //       to do so, however...
                    //
                    list_of_items = this.openDropdown_.find("li");
                    number_of_items = list_of_items.length;
                    if(max_count > number_of_items)
                    {
                        max_count = number_of_items;
                    }
                    for(; count <= max_count; ++count)
                    {
                        // to place columns horizontally, we have to use
                        // float on each li and force a width depending on
                        // the column
                        //
                        col_widths = new Array(count);
                        col_outer_widths = new Array(count);
                        list_of_items.each(function(idx, element)
                                {
                                    var col = idx % count,
                                        width = jQuery(element).width(),
                                        outer_width = jQuery(element).outerWidth(true);

                                    if(!col_widths[col]
                                    || width > col_widths[col])
                                    {
                                        // largest item in this column
                                        col_widths[col] = width;
                                    }

                                    if(!col_outer_widths[col]
                                    || outer_width > col_outer_widths[col])
                                    {
                                        // largest item in this column
                                        col_outer_widths[col] = outer_width;
                                    }
                                });
                        // get resulting width (necessary to make sure
                        // the float are moved to the next line)
                        total_width = 0;
                        jQuery.each(col_outer_widths, function(name, value)
                            {
                                total_width += value;
                            });
                        this.openDropdown_
                            .find("ul")
                            .width(total_width);
                        this.openDropdown_
                            .find("li")
                            .css("float", "left")
                            .width(function(idx, element)
                                {
                                    var col = idx % count;

                                    return col_widths[col];
                                });

                        width = this.openDropdown_.outerWidth(false);
                        if(width > screen_width)
                        {
                            break;
                        }
                        if(pos.top + parseFloat(w.css("border-top-width")) - this.openDropdown_.outerHeight(true) > scroll_top)
                        {
                            pos.top += parseFloat(w.css("border-top-width")) - this.openDropdown_.outerHeight(true);
                            if(pos.left + width > screen_width)
                            {
                                // adjust the left position so we see the right side of the dropdown
                                pos.left = screen_width - width;
                                if(pos.left < 0)
                                {
                                    pos.left = 0;
                                }
                            }
                            valid = true;
                            break;
                        }
                    }
                    if(!valid)
                    {
                        // remove the column css on failures
                        this.openDropdown_.find("ul").css("width", "");
                        this.openDropdown_.find("li").css("width", "").css("float", "");
                    }
                    break;

                case "bottom-vertical-columns":
                    if(algorithm[1] == "*")
                    {
                        // try until column count == number of items
                        // or the width fails
                        max_count = 10;
                        count = 2;
                    }
                    else
                    {
                        // exact count (count[-max])
                        column_range = algorithm[1].split("-");
                        count = parseInt(column_range[0], 10);
                        if(column_range.length == 1)
                        {
                            max_count = count;
                        }
                        else
                        {
                            max_count = parseInt(column_range[1], 10);
                        }
                    }
                    number_of_items = this.openDropdown_.find("li").length;
                    if(max_count > number_of_items)
                    {
                        max_count = number_of_items;
                    }
                    for(; count <= max_count; ++count)
                    {
                        this.openDropdown_.find("ul").css("column-count", count);
                        width = this.openDropdown_.outerWidth(false);
                        if(width > screen_width)
                        {
                            break;
                        }
                        if(pos.top + w.outerHeight(false) + this.openDropdown_.outerHeight(false) - scroll_top < screen_height)
                        {
                            pos.top += w.outerHeight(false);
                            if(pos.left + width > screen_width)
                            {
                                // adjust the left position so we see the right side of the dropdown
                                pos.left = screen_width - width;
                                if(pos.left < 0)
                                {
                                    pos.left = 0;
                                }
                            }
                            valid = true;
                            break;
                        }
                    }
                    if(!valid)
                    {
                        // remove the column-count on failures
                        this.openDropdown_.find("ul").css("column-count", "");
                    }
                    break;

                case "top-vertical-columns":
                    if(algorithm[1] == "*")
                    {
                        // try until column count == number of items
                        // or the width fails
                        max_count = 10;
                        count = 2;
                    }
                    else
                    {
                        // exact count (count[-max])
                        column_range = algorithm[1].split("-");
                        count = parseInt(column_range[0], 10);
                        if(column_range.length == 1)
                        {
                            max_count = count;
                        }
                        else
                        {
                            max_count = parseInt(column_range[1], 10);
                        }
                    }
                    number_of_items = this.openDropdown_.find("li").length;
                    if(max_count > number_of_items)
                    {
                        max_count = number_of_items;
                    }
                    for(; count <= max_count; ++count)
                    {
                        this.openDropdown_.find("ul").css("column-count", count);
                        width = this.openDropdown_.outerWidth(false);
                        if(width > screen_width)
                        {
                            break;
                        }
                        if(pos.top + parseFloat(w.css("border-top-width")) - this.openDropdown_.outerHeight(true) > scroll_top)
                        {
                            pos.top += parseFloat(w.css("border-top-width")) - this.openDropdown_.outerHeight(true);
                            if(pos.left + width > screen_width)
                            {
                                // adjust the left position so we see the right side of the dropdown
                                pos.left = screen_width - width;
                                if(pos.left < 0)
                                {
                                    pos.left = 0;
                                }
                            }
                            valid = true;
                            break;
                        }
                    }
                    if(!valid)
                    {
                        // remove the column-count on failures
                        this.openDropdown_.find("ul").css("column-count", "");
                    }
                    break;

                case "bottom-horizontal-columns-with-scrollbar":
                    if(algorithm[1] == "*")
                    {
                        // try until column count == 1
                        // and if the width fails, it completely fails
                        count = 5;
                    }
                    else
                    {
                        // user specifies how many columns to start with
                        count = parseInt(algorithm[1], 10);
                        // limit to a maximum of 20 attempts
                        if(count > 20)
                        {
                            count = 20;
                        }
                    }
                    number_of_items = this.openDropdown_.find("li").length;
                    if(count > number_of_items)
                    {
                        count = number_of_items;
                    }
                    for(; count > 0; --count)
                    {
                        // to place columns horizontally, we have to use
                        // float on each li and force a width depending on
                        // the column
                        //
//console.log(new Date);
                        tallest_line = 0;
                        col_widths = new Array(count);
                        col_outer_widths = new Array(count);
                        this.openDropdown_
                            .find("li")
                            .each(function(idx, element)
                                {
                                    var col = idx % count,
                                        width = jQuery(element).width(),
                                        outer_width = jQuery(element).outerWidth(true),
                                        outer_height = jQuery(element).outerHeight(true);

                                    if(!col_widths[col]
                                    || width > col_widths[col])
                                    {
                                        // largest item in this column
                                        col_widths[col] = width;
                                    }

                                    if(!col_outer_widths[col]
                                    || outer_width > col_outer_widths[col])
                                    {
                                        // largest item in this column
                                        col_outer_widths[col] = outer_width;
                                    }

                                    if(outer_height > tallest_line)
                                    {
                                        tallest_line = outer_height;
                                    }
                                });
//console.log(new Date);
                        // get resulting width (necessary to make sure
                        // the float are moved to the next line)
                        total_width = 0;
                        jQuery.each(col_outer_widths, function(name, value)
                            {
                                total_width += value;
                            });
                        this.openDropdown_
                            .find("ul")
                            .width(total_width + 20);   // +20 to account for the scrollbar width
                        this.openDropdown_
                            .find("li")
                            .css("float", "left")
                            .width(function(idx, element)
                                {
                                    var col = idx % count;

                                    return col_widths[col];
                                });

                        width = this.openDropdown_.outerWidth(false);
                        if(width > screen_width)
                        {
                            // too large, try with less columns
                            continue;
                        }
                        if(pos.top + w.outerHeight(false) + tallest_line - scroll_top < screen_height)
                        {
                            pos.top += w.outerHeight(false);
                            if(pos.left + width > screen_width)
                            {
                                // adjust the left position so we see the right side of the dropdown
                                pos.left = screen_width - width;
                                if(pos.left < 0)
                                {
                                    pos.left = 0;
                                }
                            }
                            dropdown_height = screen_height - (pos.top - scroll_top);
                            this.openDropdown_.height(dropdown_height);

                            // now scroll toward the existing selection if any
                            selected = this.openDropdown_.find("li.selected");
                            if(selected.exists())
                            {
                                // by setting the overflow to hidden, we will force the scrollTop()
                                // to be recalculated/used as expected (may be required for mobile phones)
                                //this.openDropdown_.find("ul").css("overflow-y", "hidden");
                                this.openDropdown_.scrollTop(0);
                                offset = (dropdown_height - selected.outerHeight()) / 2;
                                if(offset < 0)
                                {
                                    offset = 0;
                                }
                                this.openDropdown_.scrollTop(selected.position().top - offset);
                                //this.openDropdown_.find("ul").css("overflow-y", "");
                            }

                            valid = true;
                            break;
                        }
                    }
                    if(!valid)
                    {
                        // remove the column css on failures
                        this.openDropdown_
                                .css("height", "")
                                .find("ul").css("width", "")
                                .find("li").css("width", "").css("float", "");
                    }
                    break;

                case "bottom-vertical-columns-with-scrollbar":
                    if(algorithm[1] == "*")
                    {
                        // try until column count == number of items
                        // or the width fails
                        count = 5;
                    }
                    else
                    {
                        // user specified how many columns to start with
                        count = parseInt(algorithm[1], 10);
                        if(count > 20)
                        {
                            count = 20;
                        }
                    }
                    number_of_items = this.openDropdown_.find("li").length;
                    if(count > number_of_items)
                    {
                        count = number_of_items;
                    }
                    for(; count > 0; --count)
                    {
                        this.openDropdown_.find("ul").css("column-count", count);
                        if(this.openDropdown_.outerWidth(false) > screen_width)
                        {
                            break;
                        }
                        // all lines have the same height assuming that the
                        // user has no <br/>, images, etc.
                        // (we actually need TWO lines to get a scrollbar)
                        tallest_line = this.openDropdown_.find("li").outerHeight() * 2
                                        + parseFloat(this.openDropdown_.css("border-top-width"))
                                        + parseFloat(this.openDropdown_.css("border-bottom-width"));
//console.log(new Date);
//                        this.openDropdown_ -- somehow this takes 5 seconds over 465 elements... dead slow!
//                            .find("li")
//                            .each(function(idx, element)
//                                {
//                                    var outer_height = jQuery("element").outerHeight();
//
//                                    if(outer_height > tallest_line)
//                                    {
//                                        tallest_line = outer_height;
//                                    }
//                                });
//console.log(new Date);
                        width = this.openDropdown_.outerWidth(false);
                        if(width + 20 > screen_width) // +20 to account for the scrollbar width (TODO: calculate exact size)
                        {
                            // too large, try with less columns
                            continue;
                        }
                        if(pos.top + w.outerHeight(false) + tallest_line - scroll_top < screen_height)
                        {
                            pos.top += w.outerHeight(false);
                            if(pos.left + width > screen_width)
                            {
                                // adjust the left position so we see the right side of the dropdown
                                pos.left = screen_width - width - 20; // -20 to account for the scrollbar (TODO: calculate exact size)
                                if(pos.left < 0)
                                {
                                    pos.left = 0;
                                }
                            }
                            dropdown_height = screen_height - (pos.top - scroll_top);
                            if(dropdown_height > this.openDropdown_.outerHeight(false))
                            {
                                this.openDropdown_
                                        .width(width + 20);     // +20 to account for the scrollbar width (TODO: calculate exact size)
                            }
                            else
                            {
                                this.openDropdown_
                                        .height(dropdown_height)
                                        .width(width + 20);     // +20 to account for the scrollbar width (TODO: calculate exact size)
                            }

                            // now scroll toward the existing selection if any
                            selected = this.openDropdown_.find("li.selected");
                            if(selected.exists())
                            {
                                // by setting the overflow to hidden, we will force the scrollTop()
                                // to be recalculated/used as expected (may be required for mobile phones)
                                //this.openDropdown_.find("ul").css("overflow-y", "hidden");
                                this.openDropdown_.scrollTop(0);
                                offset = (dropdown_height - selected.outerHeight()) / 2;
                                if(offset < 0)
                                {
                                    offset = 0;
                                }
                                this.openDropdown_.scrollTop(selected.position().top - offset);
                                //this.openDropdown_.find("ul").css("overflow-y", "");
                            }

                            valid = true;
                            break;
                        }
                    }
                    if(!valid)
                    {
                        // remove the column-count on failures
                        this.openDropdown_
                                .find("ul").css("column-count", "");
                    }
                    break;

                case "top-vertical-columns-with-scrollbar":
                    if(algorithm[1] == "*")
                    {
                        // try until column count == number of items
                        // or the width fails
                        count = 5;
                    }
                    else
                    {
                        // user specified how many columns to start with
                        count = parseInt(algorithm[1], 10);
                        if(count > 20)
                        {
                            count = 20;
                        }
                    }
                    number_of_items = this.openDropdown_.find("li").length;
                    if(count > number_of_items)
                    {
                        count = number_of_items;
                    }
                    for(; count > 0; --count)
                    {
                        this.openDropdown_.find("ul").css("column-count", count);
                        if(this.openDropdown_.outerWidth(false) > screen_width)
                        {
                            break;
                        }
                        // all lines have the same height assuming that the
                        // user has no <br/>, images, etc.
                        tallest_line = this.openDropdown_.find("li").outerHeight() * 2
                                        + parseFloat(this.openDropdown_.css("border-top-width"))
                                        + parseFloat(this.openDropdown_.css("border-bottom-width"));
//console.log(new Date);
//                        this.openDropdown_ -- somehow this takes 5 seconds over 465 elements... dead slow!
//                            .find("li")
//                            .each(function(idx, element)
//                                {
//                                    var outer_height = jQuery("element").outerHeight();
//
//                                    if(outer_height > tallest_line)
//                                    {
//                                        tallest_line = outer_height;
//                                    }
//                                });
//console.log(new Date);
                        width = this.openDropdown_.outerWidth(false);
                        if(width > screen_width)
                        {
                            // too large, try with less columns
                            continue;
                        }
                        if(pos.top + parseFloat(w.css("border-top-width")) - tallest_line > scroll_top)
                        {
                            start_top = pos.top + parseFloat(w.css("border-top-width"));
                            pos.top += parseFloat(w.css("border-top-width")) - this.openDropdown_.outerHeight(true);
                            if(pos.top < scroll_top)
                            {
                                pos.top = scroll_top;
                            }
                            if(pos.left + width > screen_width)
                            {
                                // adjust the left position so we see the right side of the dropdown
                                pos.left = screen_width - width;
                                if(pos.left < 0)
                                {
                                    pos.left = 0;
                                }
                            }
                            dropdown_height = start_top - pos.top;
                            this.openDropdown_
                                    .outerHeight(dropdown_height)
                                    .width(this.openDropdown_.outerWidth(false) + 20);     // +20 to account for the scrollbar width (TODO: calculate exact size)

                            // now scroll toward the existing selection if any
                            selected = this.openDropdown_.find("li.selected");
                            if(selected.exists())
                            {
                                // TBD: by setting the overflow to hidden, we will force the scrollTop()
                                // to be recalculated/used as expected (may be required for mobile phones)
                                //this.openDropdown_.find("ul").css("overflow-y", "hidden");
                                this.openDropdown_.scrollTop(0);
                                offset = (dropdown_height - selected.outerHeight()) / 2;
                                if(offset < 0)
                                {
                                    offset = 0;
                                }
                                this.openDropdown_.scrollTop(selected.position().top - offset);
                                //this.openDropdown_.find("ul").css("overflow-y", "");
                            }

                            valid = true;
                            break;
                        }
                    }
                    if(!valid)
                    {
                        // remove the column-count on failures
                        this.openDropdown_
                                .find("ul").css("column-count", "");
                    }
                    break;

                }
                break;

            }
            if(valid)
            {
//console.log(new Date);
//if(typeof algorithm === "string")
//console.log("   accepted [" + algorithm + "]!!!");
//else
//console.log("   accepted [" + algorithm[0] + "]!!!");
                break;
            }

            // it was not valid, remove that class
            this.openDropdown_.removeClass(new_class);
        }
    }

    this.openDropdown_.css("left", pos.left + "px");
    this.openDropdown_.css("top", pos.top + "px");
};


/** \brief Request that the current dropdown be closed.
 *
 * This function ensures that the current dropdown gets closed.
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.hideDropdown = function()
{
    var clone = null;

    if(this.openDropdown_)
    {
        // if we are in the top window, otherwise it is already hidden
        if(this.clonedDropdown_)
        {
            clone = this.openDropdown_;
        }
        else
        {
            // reset otherwise the z-index increases forever when you have
            // more than one dropdown and click one after another
            //
            this.openDropdown_.css("z-index", 0);

            // in case we have items marked as active, remove the class
            this.openDropdown_
                .children(".dropdown-selection")
                .children(".dropdown-item")
                .removeClass("active");
        }
        this.openDropdown_.fadeOut(150, function()
            {
                var c,
                    n;

                if(clone)
                {
                    clone.remove();
                }
                else
                {
                    // make sure to remove the algorithm class so that way we can
                    // avoid having two or more in the same dropdown
                    //
                    c = jQuery(this).attr("class");
                    if(c)
                    {
                        c = c.split(" ");
                        for(n in c)
                        {
                            if(c.hasOwnProperty(n))
                            {
                                if(c[n].substr(0, 10) == "algorithm-")
                                {
                                    jQuery(this).removeClass(c[n]);
                                }
                            }
                        }
                    }
                    // remove the column related CSS which we may add if we
                    // end up using vertical columns
                    //
                    jQuery(this)
                        .css("width", "").css("height", "")
                        .find("ul").css("column-count", "").css("width", "").css("height", "").removeClass("scrollbar")
                        .find("li").css("width", "").css("float", "");
                }
            });
        this.openDropdown_ = null;
    }
};


/** \brief Handle a click on a dropdown item.
 *
 * This function is called whenever the user clicks on a dropdown
 * item.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget representing the dropdown.
 * @param {jQuery} selected_item  The widget that was clicked.
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.itemClicked = function(editor_widget, selected_item)
{
    var w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        d = w.children(".dropdown-items"),
        value;

    // ignore the click if that item is disabled
    if(!selected_item.is(".disabled"))
    {
        // hide the dropdown (we could use d.toggle() but that
        // would not update the openDropdon_ variable member)
        this.hideDropdown();

        // first select the new item
        d.find("li.dropdown-item").removeClass("selected");
        if(window.self != window.top)
        {
            // "selected_item" look nice, but it is the copy in the _top
            // window and we have to mark the selected item in the list
            // present in this popup instead
            //
            d.find("li.dropdown-item").eq(selected_item.index()).addClass("selected");
        }
        else
        {
            // avoid wasting time, we know which item this is
            selected_item.addClass("selected");
        }

        // then copy the item label to the "content" (line edit)
        c.empty();
        c.append(selected_item.html());

        // finally, get the resulting value if there is one
        value = selected_item.attr("value");
        if(value)
        {
            c.attr("value", snapwebsites.castToString(value, "dropdown item value attribute"));
        }
        else
        {
            c.removeAttr("value");
        }

        // make that dropdown the currently active object
        editor_widget.focus();
        editor_widget.getEditorBase().setActiveElement(c);

        editor_widget.getEditorBase().checkModified(editor_widget);
    }
};


/** \brief Select a different item in the list of items of the dropdown.
 *
 * This function is used whenever the user goes through the list of items
 * in the dropdown using arrow up or arrow down. The selection looks very
 * similar to what it would look like if the user hovered over the list
 * of items in the dropdown list.
 *
 * The function takes one parameter indicating the direction or which
 * item should be selected next:
 *
 * \li first -- select the very first item
 * \li last -- select the very last item
 * \li up -- select the previous item in the same column or the last in the
 *           previous column or the very last item
 * \li down -- select the next item in the same column or the first in the
 *             next column or the very first item
 * \li left -- select the item on the left (multi-column only)
 * \li right -- select the item on the right (multi-column only)
 * \li top -- select the first item in this column
 * \li bottom -- select the last item in this column
 * \li 'A' to 'Z' -- select the first item with that character, or the next one
 *                   if already on such an item
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The concerned widget.
 * @param {string} direction  A direction as indicated in the notes.
 *
 * \sa resetValue()
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.selectItem = function(editor_widget, direction)
{
    var w = editor_widget.getWidget(),
        //c = editor_widget.getWidgetContent(),
        d = this.openDropdown_,
        item = d.children(".dropdown-selection").children(".dropdown-item").not(".hidden"),
        pos = item.index(item.filter(".active")),
        inc = 1,
        turn_around = false,
        that_item, // = item.eq(pos),
        columns = 1, // TODO: implement columns
        items_per_column = item.length / columns;

    if(direction == "first")
    {
        if(pos == 0)
        {
            return;
        }
        pos = 0;
    }
    else if(direction == "last")
    {
        if(pos + 1 >= item.length)
        {
            return;
        }
        pos = item.length - 1;
        inc = -1;
    }
    else if(direction == "up")
    {
        if(pos == 0)
        {
            pos = item.length - 1;
        }
        else
        {
            --pos;
        }
        inc = -1;
        turn_around = true;
    }
    else if(direction == "down")
    {
        ++pos;
        if(pos >= item.length)
        {
            pos = 0;
        }
        turn_around = true;
    }
    else if(direction == "left")
    {
        // no effect if only 1 column or already in first column
        if(pos < items_per_column)
        {
            return;
        }
        inc = -items_per_column;
        pos += inc;
    }
    else if(direction == "right")
    {
        // no effect if only 1 column or already in last column
        if(pos + items_per_column >= item.length)
        {
            return;
        }
        inc = items_per_column;
        pos += inc;
    }
    else if(direction == "top")
    {
        pos -= pos % items_per_column;
    }
    else if(direction == "bottom")
    {
        pos = pos - pos % items_per_column + items_per_column - 1;
        if(pos >= item.length)
        {
            // last column may be shorter
            pos = item.length - 1;
        }
        inc = -1;
    }
    else if(direction.length == 1)
    {
        // search for the first or next item with the same starting letter
        that_item = item.eq(pos);
        if(that_item.text().substr(0, 1).toLowerCase() == direction)
        {
            // search next item with same starting letter
            for(++pos; pos < item.length; ++pos)
            {
                that_item = item.eq(pos);
                if(that_item.text().substr(0, 1).toLowerCase() == direction)
                {
                    break;
                }
            }
        }
        else
        {
            for(pos = 0; pos < item.length; ++pos)
            {
                that_item = item.eq(pos);
                if(that_item.text().substr(0, 1).toLowerCase() == direction)
                {
                    break;
                }
            }
        }
        if(pos == item.length)
        {
            // no match or futher match, ignore
            return;
        }
    }
    else
    {
        throw new Error("unknown direction (" + direction + ")");
    }

    that_item = item.eq(pos);
    while(that_item.is(".disabled"))
    {
        pos += inc;
        if(pos < 0 || pos >= item.length)
        {
            if(!turn_around)
            {
                // could not find another widget that's not disabled
                return;
            }
            // turn around at most once
            turn_around = false;
            if(pos < 0)
            {
                pos = item.length - 1;
            }
            else //if(pos >= item.length)
            {
                pos = 0;
            }
        }
        that_item = item.eq(pos);
    }

    // change the active selection
    item.removeClass("active");
    that_item.addClass("active");
};


/** \brief Reset the value of the specified editor widget.
 *
 * This function offers a way for programmers to dynamically reset the
 * value of a dropdown widget. You should never call the editor widget
 * type function, instead use the resetValue() function of the widget you
 * want to reset the value of (it will make sure that the modified
 * flag is properly set.)
 *
 * For a dropdown, it may reset the value back to the original as if the
 * user had never selected anything. This may be a value found in the list
 * of items, or it may be a string such as "Please select an option ...".
 * If the default value is not equal to any one of the items, then the
 * function makes sure to remove the "value" attribute of the widget.
 *
 * If the default value is an empty string, then that is used. If you do
 * not want that resolution, you may want to use the setValue()
 * function instead.
 *
 * \todo
 * We want to add support for the item marked as the default. At this
 * point we only support the default string used to ask the user to
 * "Please select an entry..."
 *
 * @param {!Object} widget  The concerned widget.
 *
 * @return {boolean}  true if the value gets changed.
 *
 * \sa setValue()
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.resetValue = function(widget)
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        d = w.children(".dropdown-items"),
        item = d.children(".dropdown-selection").children(".dropdown-item"),
        reset_value = w.children(".snap-editor-dropdown-reset-value"),
        widget_change,
        result;

    // at this point the reset value cannot be a value defined in
    // the widget;
    //
    // TODO: later add support for default values (pre-selected item)
    //       when the widget itself does not otherwise offer a full
    //       reset value...

    item.removeClass("selected");

    // check whether the value was defined
    result = !!c.attr("value");

    // reset the actual line edit
    c.empty();
    c.append(reset_value.html());

    if(result)
    {
        // if "value" is defined, reset all of that and emit an event
        c.removeAttr("value");
        editor_widget.focus();
        editor_widget.getEditorBase().setActiveElement(c);

        // send an event for each change because the user
        // may want to know even if the value was not actually
        // modified
        widget_change = jQuery.Event("widgetchange",
            {
                widget: editor_widget,
                value: null  // see TODO, will be set to default once supported
            });
        w.trigger(widget_change);

        editor_widget.getEditorBase().checkModified(editor_widget);
    }

    // if the item had a value attribute, this returns true
    // (i.e. it was modified)
    return result;
};


/** \brief Save a new value in the specified editor widget.
 *
 * This function offers a way for programmers to dynamically change the
 * value of a dropdown widget. You should never call the editor widget
 * type function, instead use the setValue() function of the widget you
 * want to change the value of (it will make sure that the modified
 * flag is properly set.)
 *
 * For a dropdown, the value parameter is expected to be one of the strings
 * found in the list of items or a number. If it is a number, then that
 * item gets selected (we use floor() to round any number down). If it
 * is a string it searches the list of items for it and selects that item.
 * If two or more items have the same label, then the first one is selected.
 *
 * If the index number is too large or the specified string is not found
 * in the existing items, then nothing happens.
 *
 * \todo
 * Add support for disabled value which cannot be selected.
 *
 * @param {!Object} widget  The concerned widget.
 * @param {!Object|string|number} value  The value to be saved.
 *
 * @return {boolean}  true if the value gets changed.
 *
 * \sa resetValue()
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.setValue = function(widget, value)
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        d = w.children(".dropdown-items"),
        item = d.children(".dropdown-selection").children(".dropdown-item"),
        that_item,
        item_value,
        widget_change;

    if(typeof value === "number")
    {
        that_item = item.eq(value);
    }
    else
    {
        //
        // if not a number, search the list of items:
        //
        //   . first we check with the "value" attribute; the value
        //     attribute is authoritative so we do want to test that
        //     first; this is viewed as a strong selection since two
        //     items cannot (should not at time of writing, really)
        //     share the same value
        //
        //   . second we give the user a chance to select using the
        //     HTML of one of the items (this is a requirement since
        //     some entries may not use the "value" attribute); this
        //     is somewhat flaky since two items could have the exact
        //     same HTML and only the first one will be selected
        //

        // Note: the filter may return more than one item, in that case
        //       we simply ignore the extra items
        that_item = item.filter(function()
            {
                var e = jQuery(this),
                    attr = e.attr("value");

                // note: if the value attribute is "" (an empty string)
                //       then we algorithm fails here and anywhere else
                //       we use that technique.
                if(attr)
                {
                    // if we have an attribute, match that
                    return attr === value;
                }

                // not a match...
                return false;
            }).eq(0);

        if(!that_item.exists())
        {
            // Note: the filter may return more than one item, in that case
            //       we simply ignore the extra items
            that_item = item.filter(function()
                {
                    // no 'value' attribute, attempt to match the HTML data
                    return jQuery(this).html() === value;
                }).eq(0);
        }
    }

    // TODO: if this Dropdown supports editing (i.e. not just read-only)
    //       then this value has to become the new value anyway.

    // if already selected, in effect we are not changing anything
    if(that_item.exists() && !that_item.hasClass("selected"))
    {
        item.removeClass("selected");
        that_item.addClass("selected");

        // then copy the item label to the "content" (line edit)
        c.empty();
        c.append(that_item.html());

        // finally, get the resulting value if there is one
        item_value = that_item.attr("value");
        if(item_value)
        {
            c.attr("value", snapwebsites.castToString(value, "dropdown item value attribute"));
        }
        else
        {
            // canonicalize the undefined value
            item_value = null; // TBD should it be item_value = value; ?
            c.removeAttr("value");
        }

        // make that dropdown the currently active object
        editor_widget.focus();
        editor_widget.getEditorBase().setActiveElement(c);

        // send an event for each change because the user
        // may want to know even if the value was not actually
        // modified
        widget_change = jQuery.Event("widgetchange",
            {
                widget: editor_widget,
                value: item_value
            });
        w.trigger(widget_change);

        editor_widget.getEditorBase().checkModified(editor_widget);

        return true;
    }

    return false;
};


/** \brief Check whether the dropdown is open, if so, prevent auto-send.
 *
 * This function checks whether the dropdown is currently open. If so,
 * make sure that the default button does not get clicked.
 *
 * @return {boolean}  true if the dropdown is closed, false otherwise.
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.acceptReturnAsDefaultButton = function()
{
    return !this.isDropdownOpen();
};


/** \brief Check whether the dropdown is open, if so, prevent auto-send.
 *
 * This function checks whether the dropdown is currently open. If so,
 * make sure that the default button does not get clicked.
 *
 * @param {!Object} editor_widget  The concerned widget
 * @param {!Object.<snapwebsites.EditorWidgetTypeBase.SaveData>} data  The data to be saved.
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.saving = function(editor_widget, data)
{
    var w = editor_widget.getWidget();

    snapwebsites.EditorWidgetTypeDropdown.superClass_.saving.call(this, editor_widget, data);

    if(w.children(".snap-editor-dropdown-reset-value").data("not-a-value") == "not-a-value")
    {
        // if the user comes here and the default is marked as not-a-value
        // then the result is "" (i.e. "undefined" or "empty") and not
        // the default value...
        //
        data.result = "";
    }
};



/** \brief Editor widget type for Checkmark widgets.
 *
 * This widget defines a checkmark in the editor forms.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeCheckmark = function()
{
    snapwebsites.EditorWidgetTypeCheckmark.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeCheckmark, snapwebsites.EditorWidgetType);


/** \brief Return "checkmark".
 *
 * Return the name of the checkmark type.
 *
 * @return {string} The name of the checkmark type.
 * @override
 */
snapwebsites.EditorWidgetTypeCheckmark.prototype.getType = function() // virtual
{
    return "checkmark";
};


/** \brief Initialize the widget.
 *
 * This function initializes the checkmark widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeCheckmark.prototype.initializeWidget = function(widget) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        toggle = function()
            {
                // toggle the current value
                var checkmark = c.find(".checkmark-area");
                checkmark.toggleClass("checked");
                c.attr("value", checkmark.hasClass("checked") ? 1 : 0);

                // TBD: necessary to avoid setting the focus anew?
                //      if not then we should remove the if() statement
                //
                // Note: from what I can tell, c is a jQuery and so we
                //       should do a get on it too; at this point, it seems
                //       that calling focus() everytime is just fine.
                //
                //if(editor_widget.getEditorBase().getActiveElement().get() != c)
                //{
                    editor_widget.focus();
                //}

                // tell the editor that something may have changed
                // TODO: call the widget function which in turn tells the
                //       editor instead of re-testing all the widgets?!
                editor_widget.getEditorBase().checkModified(editor_widget);
            };

    snapwebsites.EditorWidgetTypeCheckmark.superClass_.initializeWidget.call(this, widget);

    c.keydown(function(e)
        {
            if(e.which === 0x20) // spacebar
            {
                e.preventDefault();
                e.stopPropagation();

                toggle();
            }
        });

    w.click(function(e)
        {
            if(!(jQuery(e.target).is("a")))
            {
                // the default may do weird stuff, so avoid it!
                e.preventDefault();
                e.stopPropagation();

                toggle();
            }
        });
};


// This is not necessary anymore, but I want to keep it for documentation
// purposes.
//
// * brief Change the result to just 0 or 1.
// *
// * This function changes the result of a checkmark as the value 0 or 1
// * instead of the HTML of the sub-objects. This value represents the
// * current selection (0 -- not checked, or 1 -- checked.)
// *
// * param {!Object} editor_widget  The concerned widget
// * param {snapwebsites.EditorWidgetTypeBase.SaveData} data  The data object with the HTML and result parameters.
// *
//snapwebsites.EditorWidgetTypeCheckmark.prototype.saving = function(editor_widget, data) // virtual
//{
//    snapwebsites.EditorWidgetTypeCheckmark.superClass_.saving.call(this, editor_widget, data);
//
//    data.result = edit_area.find(".checkmark-area").hasClass("checked") ? 1 : 0;
//}



/** \brief Editor widget type for Radio widgets.
 *
 * This widget defines a set of radio buttons in editor forms.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeRadio = function()
{
    snapwebsites.EditorWidgetTypeRadio.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeRadio, snapwebsites.EditorWidgetType);


/** \brief Return "radio".
 *
 * Return the name of the radio type.
 *
 * @return {string} The name of the radio type.
 * @override
 */
snapwebsites.EditorWidgetTypeRadio.prototype.getType = function() // virtual
{
    return "radio";
};


/** \brief Initialize the widget.
 *
 * This function initializes the checkmark widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeRadio.prototype.initializeWidget = function(widget) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        select_radio = function(radio_button)
            {
                // select the current radio button
                var radio = jQuery(radio_button);
                if(!radio.hasClass("selected"))
                {
                    // remove all selected
                    radio.parent().children("li").removeClass("selected");
                    // then select this radio button
                    radio.addClass("selected");
                    c.attr("value", snapwebsites.castToString(radio.attr("value"), "casting radio value to a string"));

                    // tell the editor that something may have changed
                    // TODO: call the widget function which in turn tells the
                    //       editor instead of re-testing all the widgets?!
                    editor_widget.getEditorBase().checkModified(editor_widget);
                }
            };

    snapwebsites.EditorWidgetTypeRadio.superClass_.initializeWidget.call(this, widget);

    c.keydown(function(e)
        {
            if(e.which === 0x20) // spacebar
            {
                e.preventDefault();
                e.stopPropagation();

                select_radio(c.find(":focus"));
            }
        });

    w.find("li").click(function(e)
        {
            if(!(jQuery(e.target).is("a")))
            {
                // the default may do weird stuff, so avoid it!
                e.preventDefault();
                e.stopPropagation();

                select_radio(this);
            }
        });
};



/** \brief Editor widget type for Image Box widgets.
 *
 * This widget defines an image box in the editor forms. The whole widget
 * is just and only an image. You cannot type in data and when an image is
 * dragged and dropped over that widget, it replaces the previous version
 * of the image.
 *
 * If required, the widget is smart enough to use a proportional resize
 * so the image fits the widget area.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeImageBox = function()
{
    snapwebsites.EditorWidgetTypeImageBox.superClass_.constructor.call(this);

    return this;
};


/** \brief EditorWidgetTypeImageBox inherits from EditorWidgetType.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeImageBox, snapwebsites.EditorWidgetType);


/** \brief Return "image-box".
 *
 * Return the name of the image box type.
 *
 * @return {string} The name of the image box type.
 * @override
 */
snapwebsites.EditorWidgetTypeImageBox.prototype.getType = function() // virtual
{
    return "image-box";
};


/** \brief Initialize the widget.
 *
 * This function initializes the image box widget.
 *
 * \note
 * At this point the Image Box widget do not attach to any events since all
 * the drag and drop work is done at the EditorWidgetType level. However,
 * we have a droppedImage() function (see below) which finishes the work
 * of the drag and drop implementation.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeImageBox.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        snap_editor_element = c.parents('.snap-editor'),
        background = w.children(".snap-editor-background"),
        browse_button,
        browse_input_file;

    snapwebsites.EditorWidgetTypeImageBox.superClass_.initializeWidget.call(this, widget);

    // backgrounds are positioned as "absolute" so their width
    // is "broken" and we cannot center them in their parent
    // which is something we want to do for image-box objects
    if(background.exists())
    {
        background.css("width", snapwebsites.castToNumber(w.width(), "ImageBox widget width"))
                  .css("margin-top", (snapwebsites.castToNumber(w.height(), "ImageBox widget height")
                                      - snapwebsites.castToNumber(background.height(), "ImageBox background height")) / 2);
    }

    // by default we offer the Drag & Drop area and also show a Browse
    // button to let people browse their hard drive with the normal
    // file manager instead of a drag & drop which can be annoying to some
    //
    if(snap_editor_element.hasClass("browse"))
    {
        // there is no browse button by default, we have to create it;
        // also we do not want the Browser super ugly one to appear so
        // we create that one and place it inside a div that is so
        // small that no one can see it (but it needs to exist and
        // be otherwise considered visible.)
        jQuery(
                "<div class='browse-block'>"
                + "<div class='hidden file-input'>"
                  + "<input type='file'/>"
                + "</div>"
                + "<a href='#' class='browse-button'>Browse</a>"
              + "</div>"
              )
                .prependTo(snap_editor_element);

        browse_button = snap_editor_element.find(".browse-block a.browse-button");
        browse_input_file = snap_editor_element.find(".browse-block .hidden.file-input input");

        // connect the new browse-button anchor
        browse_button.click(function(e)
            {
                e.preventDefault();
                e.stopPropagation();

                // simulate a click on the hidden file-input button
                browse_input_file.click();
            });

        // when a file is selected, on OK you get a change()
        browse_input_file.change(function(e)
            {
                if(this.files
                && this.files.length > 0)
                {
                    // TODO: add a test, in case length > 1 and the destination
                    //       widget expects exactly 1 file, then generate an
                    //       error because we cannot know which file the user
                    //       really intended to drop (maybe we could offer a
                    //       selection, assuming we do not lose the necessary
                    //       info...) We could also just have a max. # of
                    //       possible drops and if `length > max` then err.
                    //
                    that.droppedFiles_(editor_widget, this.files, true);
                }

                return false;
            });
    }
};


/** \brief Handle the dropped image.
 *
 * This function handles the dropped image by saving it in the target
 * element.
 *
 * @param {ProgressEvent} e  The element.
 * @param {Image} img  The image to insert in the destination widget.
 *
 * @override
 */
snapwebsites.EditorWidgetTypeImageBox.prototype.droppedImage = function(e, img)
{
    var saved_active_element, editor;

    e.target.snapEditorWidget.getWidgetContent().empty();
    jQuery(img).appendTo(e.target.snapEditorWidget.getWidgetContent());

    // now make sure the editor detects the change
    //
    editor = snapwebsites.EditorInstance;
    saved_active_element = editor.getActiveElement();
    editor.setActiveElement(e.target.snapEditorWidget.getWidgetContent());
    editor.checkModified(e.target.snapEditorWidget);
    editor.setActiveElement(saved_active_element);
};



/** \brief Editor widget type for Dropped File with a Preview.
 *
 * This widget defines an image box extension in the editor forms.
 * The image form is used to display a preview of the file that gets
 * dropped in it. This is an extension of the EditorWidgetTypeImageBox,
 * which itself only accepts recognized image files.
 *
 * Just like the image box widget, this widget does not allow for typing,
 * only to drag and drop a file in it. A new file dropped on this widget
 * replaces the previously attached file (it is not additive.) It also
 * includes a Browse button
 *
 * As required, the widget is smart enough to use a proportional resize
 * so the preview is made to fit the widget area.
 *
 * The widget accepts images just like the EditorWidgetTypeImageBox
 * widget does. It also can accept file formats that we can transform
 * to a preview (i.e. a PDF of which the first page will be transform
 * in a preview image.)
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeImageBox}
 * @struct
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview = function()
{
    snapwebsites.EditorWidgetTypeDroppedFileWithPreview.superClass_.constructor.call(this);

    return this;
};


/** \brief EditorWidgetTypeDroppedFileWithPreview inherits from EditorWidgetTypeImageBox.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeDroppedFileWithPreview, snapwebsites.EditorWidgetTypeImageBox);


/** \brief Return "dropped-file-with-preview".
 *
 * Return the name of the dropped file with preview type.
 *
 * Note that this widget type has 2 sub-types named
 * "dropped-image-with-preview" and "dropped-any-with-preview". However,
 * these sub-types cannot be distinguished here. Instead you have to
 * check whether it has classes "attachment" and/or "image".
 *
 * @return {string} The name of the image box type.
 * @override
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.getType = function() // virtual
{
    return "dropped-file-with-preview";
};


/** \brief Initialize the widget.
 *
 * This function initializes the image box widget (there is not much to
 * initialize in the dropped file with preview object).
 *
 * \note
 * At this point the Image Box widget do not attach to any events since all
 * the drag and drop work is done at the EditorWidgetType level. However,
 * we have a droppedImage() function (see below) which finishes the work
 * of the drag and drop implementation.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.initializeWidget = function(widget) // virtual
{
    snapwebsites.EditorWidgetTypeDroppedFileWithPreview.superClass_.initializeWidget.call(this, widget);
};


/** \brief Handle the dropped file.
 *
 * This function handles the dropped image by saving it in the target
 * element.
 *
 * \todo
 * We may want to look into generating an MD5 checksum and check that first
 * because the file may already be available on the server. Calculating an
 * MD5 in JavaScript is not that hard...
 *
 * @param {ProgressEvent} e  The event.
 *
 * @override
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.droppedAttachment = function(e)
{
    var form_data,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (e.target.snapEditorWidget),
        editor_form = editor_widget.getEditorForm(),
        w = editor_widget.getWidget(),
        session = editor_form.getSession(),
        title_widget = editor_form.getWidgetByName("title"),
        name = editor_widget.getName(),
        broken_icon = jQuery(".broken-attachment-icon"),
        icon_widget = w.children(".attachment-icon");

    // hide previous icons if the user is doing this a second time
    broken_icon.hide();
    icon_widget.hide();

    //
    // In case of an attachment, we send them to the server because the
    // browser cannot just magically generate a preview; so we create
    // a POST with the data and send that.
    //
    // (Note: the client "could" create a preview, it would just take
    // a day or two and loads of memory...)
    //
    // The data is attached to the session of this editor form. We do that
    // because we do not want to involve the main plugin responsible for
    // the form until a full POST of all the changes. We can still have a
    // hook on a callback if necessary.
    //
    // Note that in the end this means the data of an editor form come from
    // the client and the editor session system.
    //

    form_data = new FormData();
    form_data.append("_editor_session", session);
    form_data.append("_editor_save_mode", "attachment");
    if(title_widget)
    {
        form_data.append("_editor_uri", snapwebsites.EditorForm.titleToURI(title_widget.saving().result));
    }
    form_data.append("_editor_widget_names", name); // this field supports multiple names separated by commas
    form_data.append(name, e.target.snapEditorFile);

    // mark widget as processing (allows for CSS effects)
    w.addClass("processing-attachment");

    // show a "Please Wait" image
    editor_widget.showWaitImage();

    this.ajaxReason_ = "dropped_file_with_preview";
    if(!this.serverAccess_)
    {
        this.serverAccess_ = new snapwebsites.ServerAccess(this);
    }
    this.serverAccess_.setURI(snapwebsites.castToString(jQuery("link[rel='page-uri']").attr("href"), "casting href of the page-uri link to a string in snapwebsites.EditorWidgetTypeDroppedFileWithPreview.droppedAttachment()"));
    this.serverAccess_.setData(form_data);
    this.serverAccess_.send(e);
};


/*jslint unparam: true */
/** \brief Function called on AJAX success.
 *
 * This function is called if the remote access was successful. The
 * result object includes a reference to the XML document found in the
 * data sent back from the server.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.serverAccessSuccess = function(result) // virtual
{
    // the response was successful so responseXML is valid,
    // get the canonicalized path to the file we just sent
    // to the server (this is a full URI)
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (result.userdata.target.snapEditorWidget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        xml_data = jQuery(result.jqxhr.responseXML),
        attachment_path = xml_data.find("data[name='attachment-path']").text(),
        attachment_icon = snapwebsites.castToString(xml_data.find("data[name='attachment-icon']").text(), "casting the attachment icon path to a string"),
        icon_widget = w.children(".attachment-icon"),
        request,
        preview_uri = attachment_path + "/preview.jpg";

    snapwebsites.EditorWidgetTypeDroppedFileWithPreview.superClass_.serverAccessSuccess.call(this, result);

    // show the attachment icon
    if(!icon_widget.exists())
    {
        w.prepend("<div class=\"attachment-icon\"><img src=\"" + attachment_icon + "\"/></div>");
        icon_widget = w.children(".attachment-icon");
    }
    else
    {
        // make sure to display the new icon as required
        icon_widget.find("img").attr("src", attachment_icon);
        icon_widget.show();
    }

    request = new snapwebsites.ListenerRequest(
        {
            uri: preview_uri,
            success: function(request)
                {
                    var preview_widget = editor_widget.getWidgetContent(),
                        editor = snapwebsites.EditorInstance,
                        saved_active_element = editor.getActiveElement();

                    preview_widget.empty();
                    jQuery("<img src=\"" + preview_uri + "\"/>").appendTo(preview_widget);

                    // now make sure the editor detects the change
                    // (even though we do not expect to re-save this widget)
                    editor.setActiveElement(preview_widget);
                    editor.checkModified(editor_widget);
                    editor.setActiveElement(saved_active_element);
                },
            error: function(request)
                {
                    var broken_icon = jQuery(".broken-attachment-icon");

                    // show a broken image in this case
//console.log("Editor Listener Request: FAILURE!");
                    if(!broken_icon.exists())
                    {
                        w.prepend("<div class=\"broken-attachment-icon\"><img src=\"/images/mimetype/file-broken-document.png\" width=\"48\" height=\"48\"/></div>");
                    }
                    else
                    {
                        broken_icon.show();
                    }
                },
            complete: function(request)
                {
//console.log("Editor Listener Request: COMPLETE!");
                    // not waiting anymore
                    editor_widget.hideWaitImage();
                    icon_widget.hide();
                }
        });

    request.setSpeeds([10, 3]);
    snapwebsites.ListenerInstance.addRequest(request);
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Function called on AJAX error.
 *
 * This function is called if the remote access generated an error.
 * In this case errors include I/O errors, server errors, and application
 * errors. All call this function so you do not have to repeat the same
 * code for each type of error.
 *
 * \li I/O errors -- somehow the AJAX command did not work, maybe the
 *                   domain name is wrong or the URI has a syntax error.
 * \li server errors -- the server received the POST but somehow refused
 *                      it (maybe the request generated a crash.)
 * \li application errors -- the server received the POST and returned an
 *                           HTTP 200 result code, but the result includes
 *                           a set of errors (not enough permissions,
 *                           invalid data, etc.)
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.serverAccessError = function(result) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (result.userdata.target.snapEditorWidget);

    snapwebsites.EditorWidgetTypeDroppedFileWithPreview.superClass_.serverAccessError.call(this, result);

    editor_widget.hideWaitImage();
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Function called on AJAX completion.
 *
 * This function is called once the whole process is over. It is most
 * often used to do some cleanup.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.serverAccessComplete = function(result) // virtual
{
    snapwebsites.EditorWidgetTypeDroppedFileWithPreview.superClass_.serverAccessComplete.call(this, result);

    // done processing
    result.userdata.target.snapEditorWidget.getWidget().removeClass("processing-attachment");
};
/*jslint unparam: false */



/** \brief Editor widget type for Dropped File.
 *
 * This widget defines a link per file dropped over it so a client can
 * download the files.
 *
 * The widget includes a Browse button so one can use a File Manager to
 * select the file(s) to add here.
 *
 * By default the Dropped File widget accepts only one file. So dropping
 * another file over that one file will replace the existing file. It
 * is, however, possible to setup the widget to accept multiple files
 * in which case additional links are created for each file dropped or
 * selected through the File Manager.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeDroppedFile = function()
{
    snapwebsites.EditorWidgetTypeDroppedFile.superClass_.constructor.call(this);

    return this;
};


/** \brief EditorWidgetTypeDroppedFile inherits from EditorWidgetType.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeDroppedFile, snapwebsites.EditorWidgetType);


/** \brief Return "dropped-file".
 *
 * Return the name of the dropped file type.
 *
 * @return {string} The name of the image box type.
 * @override
 */
snapwebsites.EditorWidgetTypeDroppedFile.prototype.getType = function() // virtual
{
    return "dropped-file";
};


/** \brief Initialize the widget.
 *
 * This function initializes the image box widget.
 *
 * \note
 * At this point the Dropped File widget does not attach to any events
 * since all the drag and drop work is done at the EditorWidgetType
 * level. However, we have a droppedAttachment() function (see below),
 * which finishes the work of the drag and drop implementation.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeDroppedFile.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        buttons = w.find(".dropped-file-buttons"),
        upload_button = buttons.find("div.upload.button"),
        upload_input_file = buttons.find(".hidden.file-input input"),
        download_button = buttons.find("div.download.button"),
        reset_button = buttons.find("div.reset.button");

    snapwebsites.EditorWidgetTypeDroppedFile.superClass_.initializeWidget.call(this, widget);

    // connect the new browse-button anchor
    upload_button.click(function(e)
        {
            e.preventDefault();
            e.stopPropagation();

            // simulate a click on the hidden file-input button
            that.upload(editor_widget);
        });

    // when a file is selected, on OK you get a change()
    upload_input_file.change(function(e)
        {
            if(this.files
            && this.files.length > 0)
            {
                // TODO: add a test, in case length > 1 and the destination
                //       widget expects exactly 1 file, then generate an
                //       error because we cannot know which file the user
                //       really intended to drop (maybe we could offer a
                //       selection, assuming we do not lose the necessary
                //       info...) We could also just have a max. # of
                //       possible drops and if `length > max` then err.
                //
                that.droppedFiles_(editor_widget, this.files, true);
            }

            // restore focus to that widget (since it was lost to
            // the input file widget / file manager...)
            c.focus();

            return false;
        });

    download_button.click(function(e)
        {
            e.preventDefault();
            e.stopPropagation();

            that.download(editor_widget);
        });

    reset_button.click(function(e)
        {
            e.preventDefault();
            e.stopPropagation();

            that.reset(editor_widget);
        });

    c.keydown(function(e)
        {
            if(!e.shiftKey && !e.ctrlKey && !e.altKey && !e.metaKey)
            {
                switch(e.which)
                {
                case 85:   // [U]pload
                    e.preventDefault();
                    e.stopPropagation();

                    // simulate a click on the hidden file-input button
                    that.upload(editor_widget);
                    break;

                case 13:   // Enter
                case 32:   // Space
                    // The default Enter and Space can be used by
                    // switching focus to the upload_input_file object
                    // and leave the event alone (let propagation happen)
                    //e.preventDefault();
                    //e.stopPropagation();

                    // simulate a click on the hidden file-input Browse button
                    //that.upload(editor_widget);

                    // so... calling upload() fails... however, we can trick
                    // the browser into sending the keypress and keyup events
                    // to the input file widget by moving the focus to it!
                    //
                    upload_input_file.focus(); // some say that some browsers need this...
                    break;

                case 68:   // [D]ownload
                    e.preventDefault();
                    e.stopPropagation();

                    // simulate a click on the Download button
                    that.download(editor_widget);
                    break;

                case 82:   // [R]eset
                    e.preventDefault();
                    e.stopPropagation();

                    // simulate a click on the Reset button
                    that.reset(editor_widget);
                    break;

                }
            }
        });
};


/** \brief Implement the Upload button.
 *
 * This function implements the Upload button for the specified editor
 * widget.
 *
 * \todo
 * Unfortunately, at this point a click on the Upload button works,
 * but the keyboard shortcut fails. There are many entries here but
 * really, none that are any different than the solution we currently
 * use and works with the mouse. I already tested by sending a keyup,
 * keydown, and keypress with code 13 and 32 and neither worked.
 * http://stackoverflow.com/questions/210643/in-javascript-can-i-make-a-click-event-fire-programmatically-for-a-file-input
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The editor widget that
 *        had its Upload button clicked.
 */
snapwebsites.EditorWidgetTypeDroppedFile.prototype.upload = function(editor_widget)
{
    var w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        upload_button = w.find(".dropped-file-buttons div.upload.button"),
        upload_input_file = w.find(".dropped-file-buttons .hidden.file-input input");

    // ignore clicks if the button does not exist
    if(!upload_button.exists())
    {
        return;
    }

    // simulate a click on the file "Browse" button
    //
    upload_input_file.focus(); // some say that some browsers need this...
    upload_input_file.click();
    c.focus();
};


/** \brief Implement the Download button.
 *
 * This function implements the Download button for the specified editor
 * widget.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The editor widget that
 *        had its Download button clicked.
 */
snapwebsites.EditorWidgetTypeDroppedFile.prototype.download = function(editor_widget)
{
    var uri = editor_widget.getValue(),
        w = editor_widget.getWidget(),
        download_button = w.find(".dropped-file-buttons div.download.button");

    // ignore clicks if the button is disabled
    if(download_button.hasClass("disabled")
    || uri.length == 0)
    {
        return;
    }

    // simulate a click on an anchor, but also request that
    // attachment plugin to send us the file as an attachment
    // (i.e. do not load the file directly in the browser if
    // at all possible--although the user may have an extension
    // to view the file in the browser...)
    //
    window.location = snapwebsites.ServerAccess.appendQueryString(uri, { download: "attachment" });
};


/** \brief Reset the Dropped File widget.
 *
 * We added a Reset button because there is otherwise no way to remove
 * an uploaded file from a Dropped File widget. This function implements
 * that functionality.
 *
 * The reset function, in this case, clears the widget (opposed to the
 * EditorWidget.resetValue() function which restores the last value that
 * was loaded in the form.)
 *
 * @param {snapwebsites.EditorWidget} editor_widget  Editor widget that
 *        is to be reset.
 */
snapwebsites.EditorWidgetTypeDroppedFile.prototype.reset = function(editor_widget)
{
    var w = editor_widget.getWidget(),
        icon_widget = w.find(".dropped-file-icon"),
        icon_img = icon_widget.find("img"),
        download_button = w.find(".dropped-file-buttons div.download.button"),
        reset_button = w.find(".dropped-file-buttons div.reset.button");

    // ignore clicks if the button does not exist
    if(!reset_button.exists())
    {
        return;
    }

    // restore the icon to the default
    icon_img.attr("src", /** @type {string} */ (icon_img.data("original")));
    icon_widget.removeClass("has-attachment");

    // make sure to disable the download button again
    download_button.addClass("disabled");

    // reset the value to empty
    editor_widget.setValue("", true);
};


/** \brief Save a new value in the specified editor widget.
 *
 * This function offers a way for programmers to dynamically change the
 * value of a widget. You should never call the editor widget type
 * function, instead use the setValue() function of the widget you
 * want to change the value of (it will make sure that the modified
 * flag is properly set.)
 *
 * Depending on the type, the value may be a string, a number, of some
 * other type, this is why here it is also marked as an object.
 *
 * @param {!Object} widget  The concerned widget.
 * @param {!Object|string|number} value  The value to be saved.
 *
 * @return {boolean}  true if the value gets changed.
 */
snapwebsites.EditorWidgetTypeDroppedFile.prototype.setValue = function(widget, value)
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        filename = c.find(".dropped-file-filename"),
        pos = /** @type {string} */ (value).lastIndexOf('/'),
        basename = pos < 0 ? /** @type {string} */ (value) : /** @type {string} */ (value).substr(pos + 1),
        widget_change;

    if(filename.html() !== value)
    {
        filename.empty();
        filename.append(basename);
        if(/** @type {string} */ (value).length == 0)
        {
            filename.removeAttr("title");
        }
        else
        {
            filename.attr("title", /** @type {string} */ (value));
        }

        // we also have to save that in the value attribute so the
        // standard getValue() works as expected
        //
        c.attr("value", /** @type {string} */ (value));

        // send an event for each change because the user
        // may want to know even if the value was not actually
        // modified
        widget_change = jQuery.Event("widgetchange",
            {
                widget: editor_widget,
                value: value
            });
        w.trigger(widget_change);

        editor_widget.getEditorBase().checkModified(editor_widget);

        return true;
    }

    return false;
};


/** \brief Handle the dropped file.
 *
 * This function handles the dropped image by saving it in the target
 * element.
 *
 * \todo
 * We may want to look into generating an MD5 checksum and check that first
 * because the file may already be available on the server. Calculating an
 * MD5 in JavaScript is not that hard...
 *
 * @param {ProgressEvent} e  The event.
 *
 * @override
 */
snapwebsites.EditorWidgetTypeDroppedFile.prototype.droppedAttachment = function(e)
{
    var form_data,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (e.target.snapEditorWidget),
        editor_form = editor_widget.getEditorForm(),
        w = editor_widget.getWidget(),
        session = editor_form.getSession(),
        title_widget = editor_form.getWidgetByName("title"),
        name = editor_widget.getName(),
        broken_icon = jQuery(".broken-attachment-icon"),
        icon_widget = w.children(".dropped-file-icon");

    // hide previous icons if the user is doing this a second time
    broken_icon.hide();
    icon_widget.hide();

    //
    // In case of an attachment, we send them to the server because the
    // browser cannot just magically generate a preview; so we create
    // a POST with the data and send that.
    //
    // (Note: the client "could" create a preview, it would just take
    // a day or two and loads of memory...)
    //
    // The data is attached to the session of this editor form. We do that
    // because we do not want to involve the main plugin responsible for
    // the form until a full POST of all the changes. We can still have a
    // hook on a callback if necessary.
    //
    // Note that in the end this means the data of an editor form come from
    // the client and the editor session system.
    //

    form_data = new FormData();
    form_data.append("_editor_session", session);
    form_data.append("_editor_save_mode", "attachment");
    if(title_widget)
    {
        form_data.append("_editor_uri", snapwebsites.EditorForm.titleToURI(title_widget.saving().result));
    }
    form_data.append("_editor_widget_names", name); // this field supports multiple names separated by commas
    form_data.append(name, e.target.snapEditorFile);

    // mark widget as processing (allows for CSS effects)
    w.addClass("processing-attachment");

    // show a "Please Wait" image
    editor_widget.showWaitImage();

    this.ajaxReason_ = "dropped_file";
    if(!this.serverAccess_)
    {
        this.serverAccess_ = new snapwebsites.ServerAccess(this);
    }
    this.serverAccess_.setURI(snapwebsites.castToString(jQuery("link[rel='page-uri']").attr("href"), "casting href of the page-uri link to a string in snapwebsites.EditorWidgetTypeDroppedFile.droppedAttachment()"));
    this.serverAccess_.setData(form_data);
    this.serverAccess_.send(e);
};


/** \brief Function called on AJAX success.
 *
 * This function is called if the remote access was successful. The
 * result object includes a reference to the XML document found in the
 * data sent back from the server.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.EditorWidgetTypeDroppedFile.prototype.serverAccessSuccess = function(result) // virtual
{
    // the response was successful so responseXML is valid,
    // get the canonicalized path to the file we just sent
    // to the server (this is a full URI)
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (result.userdata.target.snapEditorWidget),
        w = editor_widget.getWidget(),
        xml_data = jQuery(result.jqxhr.responseXML),
        attachment_path = xml_data.find("data[name='attachment-path']").text(),
        attachment_icon = snapwebsites.castToString(xml_data.find("data[name='attachment-icon']").text(), "casting the attachment icon path to a string"),
        icon_widget = w.find(".dropped-file-icon"),
        download_button = w.find(".dropped-file-buttons div.download.button");

    snapwebsites.EditorWidgetTypeDroppedFile.superClass_.serverAccessSuccess.call(this, result);

    // show the attachment icon
    icon_widget.find("img").attr("src", attachment_icon);
    icon_widget.addClass("has-attachment");

    download_button.removeClass("disabled");

    // TODO: add support for any number of attachments
    //
    editor_widget.setValue(attachment_path, true);
};


/** \brief Function called on AJAX completion.
 *
 * This function is called once the whole process is over. It is most
 * often used to do some cleanup.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.EditorWidgetTypeDroppedFile.prototype.serverAccessComplete = function(result) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (result.userdata.target.snapEditorWidget);

    snapwebsites.EditorWidgetTypeDroppedFile.superClass_.serverAccessComplete.call(this, result);

    editor_widget.hideWaitImage();

    // done processing
    result.userdata.target.snapEditorWidget.getWidget().removeClass("processing-attachment");
};



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.EditorInstance = new snapwebsites.Editor();
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeHidden());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeTextEdit());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeLineEdit());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeDropdown());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeCheckmark());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeRadio());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeImageBox());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeDroppedFileWithPreview());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeDroppedFile());
    });

// vim: ts=4 sw=4 et
