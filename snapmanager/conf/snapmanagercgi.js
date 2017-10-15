/*
 * Name: snapmanagercgi.js
 * Layout: default
 * Version: 0.2
 * Browsers: all
 * Copyright: Copyright 2017 (c) Made to Order Software Inc.
 * License: GPLv2
 */

// Find first ancestor of element with tagName
// or undefined if not found
//
function upTo(el, tagName)
{
    tagName = tagName.toLowerCase();
    while (el && el.parentNode)
    {
        el = el.parentNode;
        if (el.tagName && el.tagName.toLowerCase() === tagName)
        {
            return el;
        }
    }
    return null;
}



function spin_globe(rotate)
{
    var selector = jQuery("img[id='globe']");
    if( rotate )
    {
        selector.attr("src","globe.gif");
    }
    else
    {
        selector.attr("src","globe_still.png");
    }
}


// Forward declare functions
//
function hook_up_form_events()       {}


var div_map = {};

function FieldDiv( the_form )
{
    var jq_form         = jQuery(the_form);
    this.parent_tr      = upTo(the_form,"tr");
    this.parent_div     = jQuery( upTo(this.parent_tr,"div") );
    this.button_name    = jq_form.data("button_name");
    this.form_post_data = jq_form.serialize();
    this.form_data      = {};
 
    var local_this = this;
    jQuery.each( jq_form.serializeArray(), function(i,element) {
        local_this.form_data[element.name] = element.value;
    });

    jQuery(this.parent_tr).addClass("modified");
    spin_globe(true);

    // Replace the div which contiains the "modified" tr.
    // If you set save_form_data to 'true', it will first
    // save the embedded form data, useful for POSTs.
    //
    this.replace_div = function( response )
    {
        this.parent_div.html(response);
    }

    this.get_field_id = function()
    {
        return this.form_data["plugin_name"] + "::" + this.form_data["field_name"];
    }

    this.is_modified = function()
    {
        return this.parent_div.find("tr[class='modified']").length > 0;
    }

    this.get_post_data = function()
    {
        return this.form_post_data + "&" + this.button_name + "=";
    }

    this.check_status = function()
    {
        hook_up_form_events();
        if( this.is_modified() )
        {
            spin_globe(true);
            this.button_name = "status";
            var that = this;
            setTimeout( function() { that.emit_ajax_request(); }, 1000 ); // Check status after 1 second.
        }
        else
        {
            spin_globe(false);
        }
    }

    this.emit_ajax_request = function()
    {
        var that = this;
        jQuery.ajax(
        {
            url : "snapmanager",
            type: "POST",
            data: that.get_post_data()
        })
        .done( function(response)
        {
            that.replace_div( response );
            that.check_status();
        })
        .fail( function( xhr, the_status, errorThrown )
        {
            console.log( "Failed to connect to server!"  );
            console.log( "xhr   : [" + xhr         + "]" );
            console.log( "status: [" + status      + "]" );
            console.log( "error : [" + errorThrown + "]" );
            //
            that.check_status();
        });
    }
}


function check_for_modified_divs()
{
    var pending = false;
    jQuery.each( div_map, function( index, div_object )
    {
        if( div_object.is_modified() )
        {
            pending = true;
        }
    });
    return pending;
}


// Hook up form events.
//
// For new DOM objects we injected, this is imperative.
//
function hook_up_form_events()
{
    jQuery("button").click( function(event)
    {
        if( check_for_modified_divs() )
        {
            // Ignore button hit if operation pending.
            event.preventDefault();
            return;
        }

        // Get the button name that was hit, find the form,
        // and set the name of the button in the form's user
        // data. This way we can retrieve it when creating the
        // FieldDiv object in the form submit below.
        //
        var button_name = jQuery(this).attr("name");
        var parent_form = jQuery(this).parent();
        parent_form.data( "button_name", button_name );
    });

    jQuery(".manager_form").submit( function( event )
    {
        // Don't submit the HTML form as normal--we will use AJAX instead.
        //
        event.preventDefault();

        // Create the FieldDiv object, remember it in the map,
        // and emit the ajax.
        //
        // We put it into the map so we only ever have one, and
        // we can find it later.
        //
        var div_object = new FieldDiv( this );
        div_map[div_object.get_field_id()] = div_object;

        div_object.emit_ajax_request();
    });
}


// When the document is ready, move the ul and divs into jQuery tabs.
//
// Hook up all form events, so when we click on a button, an Ajax POST
// is sent to the server.
//
jQuery(document).ready(function()
{
    jQuery("#tabs").tabs(
    {
        heightStyle: "content",
    }); 

    jQuery( "#menu" ).menu(
    {
        classes:
        {
            "ui-menu": "highlight"
        }
    });

    spin_globe( jQuery("tr[class='modified']").length > 0 );

    hook_up_form_events();
});

// vim: ts=4 sw=4 et
