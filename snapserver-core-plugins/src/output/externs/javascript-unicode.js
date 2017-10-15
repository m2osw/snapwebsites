/*
 * Name: javascript-unicode
 * Version: 1.0.0
 * Browsers: all
 * Copyright: Copyright 2014-2017 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
 */

/**
 *
 * Although these are "externs" they are part of snapwebsites which is
 * not declared as an extern so we cannot mark the file as such...
 *
 * externs
 */


/** \brief Get the category of a Unicode character.
 *
 * This function searches for the category of the specified UTF-32 unicode
 * character.
 *
 * The list of categories currently understood:
 *
 * \li Mn -- Non-spacing Mark
 * \li Mc -- Spacing Mark
 * \li Me -- Enclosing Mark
 * \li Nd -- Decimal Number
 * \li Nl -- Letter Number
 * \li No -- Other Number
 * \li Zs -- Space Separator
 * \li Zl -- Line Separator
 * \li Zp -- Paragraph Separator
 * \li Cc -- Control
 * \li Cf -- Format
 * \li Co -- Private Use
 * \li Cn -- Unassigned
 * \li Lu -- Uppercase Letter
 * \li Ll -- Lowercase Letter
 * \li Lt -- Titlecase Letter
 * \li Lm -- Modified Letter
 * \li Lo -- Other Letter
 * \li Pc -- Connector Punctuation
 * \li Pd -- Dash Punctuation
 * \li Ps -- Open Punctuation
 * \li Pe -- Close Punctuation
 * \li Pi -- Initial Punctuation
 * \li Pf -- Final Punctuation
 * \li Po -- Other Punctuation
 * \li Sm -- Math Symbol
 * \li Sc -- Currency Symbol
 * \li Sk -- Modifier Symbol
 * \li So -- Other Symbol
 *
 * http://www.unicode.org/reports/tr44/
 *
 * \note
 * The system makes use of the Unicode definition from the version of
 * Qt that is in use. Although the javascript-unicode.js is generated
 * at compile time.
 *
 * @param {number} ucs4  The UTF-32 character to convert to a category.
 *
 * @return {string}  The name of the category such as Cc or Lt.
 */
snapwebsites.unicodeCategory = function(ucs4) {};


/** \brief Get an array of the code points of a string.
 *
 * This function covers all the characters of a string to UTF-32
 * characters. The result is an array with the code point (UTF-32)
 * characters extracted.
 *
 * The function is aware of the UTF-16 encoding used by JavaScript and
 * will properly convert letters to UTF-32.
 *
 * @param {string} s  The string to conver to an array of code points.
 *
 * @return {Array.<number>}  An array of UTF-32 numbers.
 */
snapwebsites.stringToUnicodeCodePoints = function(s) {};


/** \brief Get the category of each character in a string.
 *
 * This function converts a string to an array of characters and
 * then converts each character to its Unicode category and
 * returns that array as the result.
 *
 * @param {string} s  The string to convert to an array of categories.
 *
 * @return {Array.<string>}  An array of categories.
 */
snapwebsites.stringToUnicodeCategories = function(s) {};
