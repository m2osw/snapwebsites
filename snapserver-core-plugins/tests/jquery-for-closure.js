/*
 * Fake definitions to run the Google Closure Compiler against our code
 * to discover errors that their compile detects.
 *
 * Command line (please update as we go):
 * java -jar ../tmp/google-js-compiler/compiler.jar --warning_level VERBOSE \
 * 	--externs ../tmp/google-js-compiler/closure-compiler/contrib/externs/jquery-1.9.js \
 * 	--js_output_file /dev/null \
 * 	plugins/output/jquery-extensions.js \
 * 	plugins/output/output.js plugins/output/popup.js \
 * 	plugins/editor/editor.js
 *
 * Google closure annotations:
 * https://developers.google.com/closure/compiler/docs/js-for-compiler
 */

/**
 * @param {string|Object} selector  The CSS like selector.
 * @param {Object=} opt_context  The context, defaults to document.
 */
function jQuery(selector, opt_context)
{
	//return new jQuery.prototype.init(selector, opt_context);
	return null;
}

jQuery.ajax = function(a, b) { };
jQuery.prototype = {
	addClass: function(a, b) { },
	animate: function(a, b) { },
	appendTo: function(a) { },
	attr: function(a, b) { },
	children: function(a) { },
	css: function(selector, opt_new_value) { },
	delay: function(a) { },
	each: function(a) { },
	fadeIn: function(a) { },
	fadeOut: function(a) { },
	hasClass: function(a) { },
	html: function() { },
	hover: function(a) { },
	init: function(selector, opt_context) { },
	keydown: function(a) { },
	mouseleave: function() { },
	off: function(a) { },
	on: function(a, b) { },
	parents: function(a) { },
	prependTo: function(a) { },
	prop: function(a) { },
	ready: function(a) { },
	removeAttr: function(a) { },
	removeClass: function(a) { },
	val: function() { }
};

