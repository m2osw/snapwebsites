
This plugin makes use of the Magdir directory found in the magic project.
(Note: now that they compile the magic files, the Magdir is only found in
the library source files and not under /etc.) At this time we make a copy
of that directory here in this plugin to make it easy to compile our
plugin.

At this point all those files are from the magic project. We may add our
own files, in which case we will put them under a different directory.

We also create 3 groups of files:

 . group of overly used file formats called the "basics" formats.
 . group of commonly used file format called the "normal" formats.
 . group of rarely used file format called the "advanced" formats.

This is done that way because the file formats being tested need to be
sent to the client. Even if you have one single .js file, if you can
make it very small, it will make everything faster. These groups appear
in the CMakeLists.txt file as the lists are defined to compile each
one to a specific .js file.

The version of the .js file is defined in the CMakeLists.txt file: search
for "MIMETYPE_VERSION_". That version is saved in the magic-to-js.h file
and saved in the JavaScript files by the magic-to-js tool.

TODO:

  * Test that most of the magic files work. (Only tested PDF for now!)
  * Add support to handle the indirect entries.
  * See whether we want to detect file formats, even if no MIME type is
    defined. (This is not really part of Snap! Core).

Note:

  This folder includes a .gitignore because the .js files are generated
  and they must not end up in the git repository (although that could
  be a good idea too, but if you really want the files, you can extract
  them from the .deb packages, even if you are not on Debian or Ubuntu
  because you can use 'ar' and then 'tar' to extract files from a
  .deb package.)

