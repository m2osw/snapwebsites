/*!
 * Name: agency
 * Version: 4.0.0.13
 * Browsers: all
 * Description: Agency theme based on bootstrap.
 *
 * @preserve
 */

// Agency Theme JavaScript

jQuery(document).ready(function()
    {
        // jQuery for page scrolling feature - requires jQuery Easing plugin
        jQuery('a.page-scroll').click(function(event)
        {
            var $anchor = jQuery(this);
            jQuery('html, body')
                .stop()
                .animate(
                    {
                        // #mainNav is about 54 pixels high so we want to
                        // be "off" by 54 pixels
                        //
                        scrollTop: jQuery($anchor.attr('href')).offset().top - 54
                    },
                    1250,
                    'easeInOutExpo');
            event.preventDefault();
        });

        // Highlight the top nav as scrolling occurs
        jQuery('body').scrollspy(
            // The #mainNav is about 54 pixels but various browsers get it
            // wrong and are off by several pixels so having a larger offset
            // is "safer" (much more likely to work as expected)
            //
            {
                target: '#mainNav',
                offset: 100
            });

        // Closes the Responsive Menu on Menu Item Click
        jQuery('#navbarResponsive ul li a')
            .click(function()
                {
                    jQuery('#navbarResponsive').collapse('hide');
                });

        // jQuery to collapse the navbar on scroll
        //jQuery(window).scroll(function()
        //    {
        //        if (jQuery("#mainNav").offset().top > 100)
        //        {
        //            jQuery("#mainNav").addClass("navbar-shrink");
        //        }
        //        else
        //        {
        //            jQuery("#mainNav").removeClass("navbar-shrink");
        //        }
        //    });
        // it's possible to use affix() instead of a scroll, but I would
        // need to better understand how that works (i.e. it looks like
        // we need to have .affix CSS info in correlation...)
        jQuery("#mainNav").affix(
                {
                    offset: { top: 100 }
                });
    });

// vim: ts=4 sw=4 et
