/*
 * Name: password-externs
 * Version: 1.0.0
 * Browsers: all
 * Copyright: Copyright 2014-2017 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
 */

/**
 *
 * @externs
 */


/** \brief Minimum length of the password in characters.
 *
 * This variable represents the minimum length that a password must be
 * to be acceptable. The function that counts the number of characters
 * in the password counts them as UTF-32 characters.
 *
 * @type {number}
 */
var password__policy__minimum_length = 0;


/** \brief Minimum number of lowercase letters.
 *
 * This variable represents the minimum number of lowercase letters
 * that the password has to include. The functions counting the number
 * of letters is doing so in Unicode. So lowercase letters are not
 * limited to just 'a' to 'z'.
 *
 * @type {number}
 */
var password__policy__minimum_lowercase_letters = 0;


/** \brief Minimum number of uppercase letters.
 *
 * This variable represents the minimum number of uppercase letters
 * that the password has to include. The functions counting the number
 * of letters is doing so in Unicode. So uppercase letters are not
 * limited to just 'A' to 'Z'.
 *
 * @type {number}
 */
var password__policy__minimum_uppercase_letters = 0;


/** \brief Minimum number of letters.
 *
 * This variable represents the minimum number of letters
 * that the password has to include. The functions counting the number
 * of letters is doing so in Unicode. So letters are not
 * limited to just 'A' to 'Z' and 'a' to 'z'.
 *
 * @type {number}
 */
var password__policy__minimum_letters = 0;


/** \brief Minimum number of digits.
 *
 * This variable represents the minimum number of digits that
 * the password has to include. The function counting the number
 * of letters is doing so in Unicode. So digits are not limited
 * to just '0' to '9'.
 *
 * @type {number}
 */
var password__policy__minimum_digits = 0;


/** \brief Minimum number of space characters.
 *
 * This variable represents the minimum number of space characters
 * that the password has to include. The function counting the number
 * of spaces is doing so in Unicode. So space characters are not
 * limited to just ASCII code 0x20.
 *
 * @type {number}
 */
var password__policy__minimum_spaces = 0;


/** \brief Minimum number of special characters.
 *
 * This variable represents the minimum number of special characters
 * that the password has to include. The function counting the number
 * of letters is doing so in Unicode. So special characters are not
 * limited to just ASCII punctuation such as '!' and '?'. It includes
 * all the characters that Unicode defines as special.
 *
 * @type {number}
 */
var password__policy__minimum_specials = 0;


/** \brief Minimum number of Unicode characters.
 *
 * This variable represents the minimum number of Unicode characters
 * that the password has to include.
 *
 * Any character which have a code larger than 255 is considered a
 * Unicode character. If your language is not mainly using Latin
 * letters then this parameter is most certainly completely useless.
 *
 * @type {number}
 */
var password__policy__minimum_unicode = 0;


/** \brief Minimum variation in the different types of characters.
 *
 * This variable represents the minimum number of types of characters
 * that the password has to include.
 *
 * This number allows you to not limit passwords to specific characters
 * but still force the user to enter a password with what you consider
 * enough variations.
 *
 * @type {number}
 */
var password__policy__minimum_variation = 0;


/** \brief Minimum length of each of each variation of characters.
 *
 * This variable represents the minimum number that each types of
 * characters has to have in order to be considered valid. Only
 * `password__policy__minimum_variation` sets of characters are
 * checked and the test starts with the sets that have the most
 * characters in them. In other words, if you want the user to have
 * at least 2 sets of characters of 3 characters each, the user
 * can include 3 digits, 3 letters, and one special character to
 * satisfy the requirements.
 *
 * @type {number}
 */
var password__policy__minimum_length_of_variations = 0;


