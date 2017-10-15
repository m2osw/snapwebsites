/*!
 * Name: sb-admin-2
 * Version: 2.1.0.25
 * Browsers: all
 * Dependencies: metis-menu
 *
 * Description: Start Bootstrap - SB Admin 2 v3.3.7+1 (http://startbootstrap.com/template-overviews/sb-admin-2)
 * Copyright: Copyright 2013-2016 Start Bootstrap
 * License: Licensed under MIT (https://github.com/BlackrockDigital/startbootstrap/blob/gh-pages/LICENSE)
 *
 * @preserve
 */
$(function() {
    $('#side-menu').metisMenu();
});

//Loads the correct sidebar on window load,
//collapses the sidebar on window resize.
// Sets the min-height of #page-wrapper to window size
$(function() {
    $(window).bind("load resize", function() {
        var topOffset,
            width = this.window.innerWidth > 0 ? this.window.innerWidth : this.screen.width,
            height = (this.window.innerHeight > 0 ? this.window.innerHeight : this.screen.height) - 1;
            min_height = 0;

        jQuery(".screen-min-height").each(
                    function()
                    {
                        var h = jQuery(this).outerHeight();
                        if(min_height < h)
                        {
                            min_height = h;
                        }
                    });

        // are we on a narrow screen (i.e. smart phone)?
        $('div.navbar-collapse').toggleClass('collapse', width < 768);

        // now that the sidebar is visible or not, get the height
        topOffset = $("nav.navbar-static-top").outerHeight();

        // remove navigation height
        height -= topOffset;

        // remove footer height
        height -= $("footer.page-footer").outerHeight();

//$("h1.title").text(height + " / " + topOffset + " / " + $("footer.page-footer").outerHeight());

        // adjust height to a valid minimum (i.e. 0px == ignore)
        if(height < min_height)
        {
            height = min_height;
        }
        $("#page-wrapper").css("min-height", height + "px");

        // the header is fixed and fields get hidden unless the editor
        // knows about it
        //
        snapwebsites.EditorInstance.setFixedHeaderHeight(topOffset);
    });

    var url = window.location;
    // var element = $('ul.nav a').filter(function() {
    //     return this.href == url;
    // }).addClass('active').parent().parent().addClass('in').parent();
    var element = $('ul.nav a').filter(function() {
        return this.href == url;
    }).addClass('active').parent();

    while(true)
    {
        if(element.is('li'))
        {
            element = element.parent().addClass('in').parent();
        }
        else
        {
            break;
        }
    }
});

// vim: ts=4 sw=4 et
