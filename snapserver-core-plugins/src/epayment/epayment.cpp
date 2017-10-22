// Snap Websites Server -- handle an array of electronic payment facilities...
// Copyright (C) 2011-2017  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "epayment.h"

#include "../editor/editor.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(epayment, 1, 0)


/* \brief Get a fixed epayment name.
 *
 * The epayment plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_EPAYMENT_CANCELED_PATH:
        return "epayment/canceled";

    case name_t::SNAP_NAME_EPAYMENT_DESCRIPTION:
        return "epayment::description";

    case name_t::SNAP_NAME_EPAYMENT_FAILED_PATH:
        return "epayment/failed";

    case name_t::SNAP_NAME_EPAYMENT_GRAND_TOTAL:
        return "epayment::grand_total";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_NUMBER:
        return "epayment::invoice_number";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS:
        return "epayment::invoice_status";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_ABANDONED:
        return "abandoned";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED:
        return "canceled";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED:
        return "completed";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED:
        return "created";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED:
        return "failed";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID:
        return "paid";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING:
        return "pending";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING:
        return "processing";

    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_UNKNOWN:
        return "unknown";

    case name_t::SNAP_NAME_EPAYMENT_LONG_DESCRIPTION:
        return "epayment::long_description";

    case name_t::SNAP_NAME_EPAYMENT_PRICE:
        return "epayment::price";

    case name_t::SNAP_NAME_EPAYMENT_PRODUCT:
        return "epayment::product";

    case name_t::SNAP_NAME_EPAYMENT_PRODUCT_NAME:
        return "epayment::product_name";

    case name_t::SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH:
        return "types/taxonomy/system/content-types/epayment/product";

    case name_t::SNAP_NAME_EPAYMENT_QUANTITY:
        return "epayment::quantity";

    case name_t::SNAP_NAME_EPAYMENT_QUANTITY_MINIMUM:
        return "epayment::quantity_minimum";

    case name_t::SNAP_NAME_EPAYMENT_QUANTITY_MAXIMUM:
        return "epayment::quantity_maximum";

    case name_t::SNAP_NAME_EPAYMENT_QUANTITY_INCREMENT:
        return "epayment::quantity_increment";

    case name_t::SNAP_NAME_EPAYMENT_RECURRING:
        return "epayment::recurring";

    case name_t::SNAP_NAME_EPAYMENT_RECURRING_SETUP_FEE:
        return "epayment::recurring_setup_fee";

    case name_t::SNAP_NAME_EPAYMENT_SKU:
        return "epayment::sku";

    case name_t::SNAP_NAME_EPAYMENT_STORE_NAME:
        return "epayment::store_name";

    case name_t::SNAP_NAME_EPAYMENT_THANK_YOU_PATH:
        return "epayment/thank-you";

    case name_t::SNAP_NAME_EPAYMENT_THANK_YOU_SUBSCRIPTION_PATH:
        return "epayment/thank-you-subscription";

    case name_t::SNAP_NAME_EPAYMENT_TOTAL:
        return "epayment::total";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_EPAYMENT_...");

    }
    NOTREACHED();
}







/** \brief Create an empty value.
 *
 * This constructor creates an empty string value.
 *
 * This is used by the std::map<>() as it requires to be capable of creating
 * empty values.
 *
 * \param[in] value  The string value of this value_t object.
 */
epayment_product::value_t::value_t()
    : f_type(type_t::TYPE_STRING)
{
}


/** \brief Save a string value in this value object.
 *
 * This constructor creates a string value.
 *
 * \param[in] value  The string value of this value_t object.
 */
epayment_product::value_t::value_t(QString const & value)
    : f_type(type_t::TYPE_STRING)
    , f_string(value)
{
}


/** \brief Save an integer value in this value object.
 *
 * This constructor creates an integer value.
 *
 * \param[in] value  The integer value of this value_t object.
 */
epayment_product::value_t::value_t(int64_t const value)
    : f_type(type_t::TYPE_INTEGER)
    , f_integer(value)
{
}


/** \brief Save a float value in this value object.
 *
 * This constructor creates a float value.
 *
 * \param[in] value  The floating point value of this value_t object.
 */
epayment_product::value_t::value_t(double const value)
    : f_type(type_t::TYPE_FLOAT)
    , f_float(value)
{
}


/** \brief Retrieve the type of this value.
 *
 * A value can be given one of the following types:
 *
 * \li type_t::TYPE_STRING
 * \li type_t::TYPE_INTEGER
 * \li type_t::TYPE_FLOAT
 *
 * \return The current value type.
 */
epayment_product::type_t epayment_product::value_t::get_type() const
{
    return f_type;
}


/** \brief Retrieve the string.
 *
 * If the value is of type type_t::TYPE_STRING, this function returns
 * the string, otherwise it will throw.
 *
 * \exception epayment_invalid_type_exception
 * This exception is raised of the type of this value is not
 * type_t::TYPE_STRING.
 *
 * \param[in] name  The name of the property to retrieve.
 *
 * \return The string defined in this value.
 */
QString const& epayment_product::value_t::get_string_value(QString const & name) const
{
    if(f_type != type_t::TYPE_STRING)
    {
        throw epayment_invalid_type_exception(QString("epayment::value_t of \"%1\" is not a string").arg(name));
    }

    return f_string;
}


/** \brief Retrieve the integer.
 *
 * If the value is of type TYPE_INTEGER, this function returns the
 * integer, otherwise it will throw.
 *
 * \exception epayment_invalid_type_exception
 * This exception is raised of the type of this value is not TYPE_INTEGER.
 *
 * \return The integer defined in this value.
 */
int64_t epayment_product::value_t::get_integer_value() const
{
    if(f_type != type_t::TYPE_INTEGER)
    {
        throw epayment_invalid_type_exception("this epayment::value_t is not an integer");
    }

    return f_integer;
}


/** \brief Retrieve the float.
 *
 * If the value is of type TYPE_FLOAT, this function returns the
 * float, otherwise it will throw.
 *
 * \exception epayment_invalid_type_exception
 * This exception is raised of the type of this value is not TYPE_FLOAT.
 *
 * \return The float defined in this value.
 */
double epayment_product::value_t::get_float_value() const
{
    if(f_type != type_t::TYPE_FLOAT)
    {
        throw epayment_invalid_type_exception("this epayment::value_t is not a floating point");
    }

    return f_float;
}


/** \brief Initialize a product object.
 *
 * This function initializes a product object with the specified product
 * path and quantity.
 *
 * The function sets the epayment::product and epayment::quantity fields
 * from the specified \p product and \p quantity parameters.
 *
 * Note that if your implement your own set of products, you still need
 * to mark them as products (i.e. have a page type defined as
 * ".../epayment/product") if you want the verify_guid() function to
 * work. Having pages to represent products is not a requirement of
 * the e-Payment facility (it is for the e-Commerce that deals with a
 * cart and needs to "add something to a cart").
 *
 * \note
 * For wishlists, the quantity is necessary too because if someone else
 * is to purchase those things, the quantity needs to be the same as in the
 * cart otherwise the thrid party buyer would not know how many he has to
 * purchase.
 *
 * \warning
 * The product path or GUID is NOT checked by this function. It can be
 * checked using the verify_guid() function. It is done this way to allow
 * special carts / products that do not automatically make use of a plain
 * page to describe a product.
 *
 * \param[in] product  The path (or GUID) to the product being created.
 * \param[in] quantity  The quantity being purchased.
 */
epayment_product::epayment_product(QString const & product, double const quantity, QString const & description)
    : f_content_plugin(content::content::instance())
    , f_revision_table(f_content_plugin->get_revision_table())
{
    // we cannot call the set_property() functions because the product
    // and quantity are marked as "read-only" properties
    //set_property(get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT), product.toUtf8().data());
    value_t p(product);
    f_properties[get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT)] = p;

    //set_property(get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY), quantity);
    value_t q(quantity);
    f_properties[get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY)] = q;

    set_property(get_name(name_t::SNAP_NAME_EPAYMENT_DESCRIPTION), description.toUtf8().data());
}


/** \brief Verify the product GUID.
 *
 * This function verifies that the product path specified in the constructor
 * is indeed a valid product GUID. This means that the product exists as
 * a page and is assigned the type:
 *
 * \code
 *    /types/taxonomy/system/content-types/epayment/product
 * \endcode
 *
 * (if you want to reference the product type, please use the 
 * name_t::SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH name instead of the path directly.)
 *
 * Note that products can be given many other types, as long as these are
 * defined below the product type path. So if you are selling instruments,
 * you could mark a product as a flute by creating a sub-type such as:
 *
 * \code
 *    /types/taxonomy/system/content-types/epayment/product/flute
 * \endcode
 *
 * \return true if the GUID represents a product that the e-Payment plugin
 *         can handle.
 */
bool epayment_product::verify_guid() const
{
    if(f_verified == verification_t::VERIFICATION_NOT_DONE)
    {
        f_verified = verification_t::VERIFICATION_INVALID;

        // get the product GUID
        QString const product(get_string_property(get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT)));

        // a product must exist, so throw if the path is wrong
        // (we may want to think about that twice since this means you
        // CANNOT ever delete a product... if the product is to be reaccessed
        // by an old invoice--unless invoices get deleted after a while or
        // the link in invoices gets dropped properly.)
        f_product_ipath.set_path(product);
        if(!f_revision_table->exists(f_product_ipath.get_revision_key()))
        {
            return false;
        }
        f_revision_row = f_revision_table->getRow(f_product_ipath.get_revision_key());
        if(!f_revision_row->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
        {
            return false;
        }

        // Is this GUID pointing to a page representing a product at least?
        links::link_info product_info(content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE), true, f_product_ipath.get_key(), f_product_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(product_info));
        links::link_info product_child_info;
        if(!link_ctxt->next_link(product_child_info))
        {
            return false;
        }

        // the link_info returns a full key with domain name
        // use a path_info_t to retrieve the cpath instead
        content::path_info_t type_ipath;
        type_ipath.set_path(product_child_info.key());
        if(type_ipath.get_cpath() != get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH)
        && !type_ipath.get_cpath().startsWith(QString("%1/").arg(get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH))))
        {
            return false;
        }

        f_verified = verification_t::VERIFICATION_VALID;
        return true;
    }
    // else -- return cached result

    return f_verified == verification_t::VERIFICATION_VALID;
}


/** \brief Set a property as a string.
 *
 * This function sets the named property using the specified string as
 * the value.
 *
 * TBD -- shall we enforce the type of the property depending on its name?
 *
 * \exception epayment_cannot_set_exception
 * This function raises this exception if the property being set is
 * the product path which cannot be changed.
 *
 * \param[in] name  The name of the property to set to \p value.
 * \param[in] value  The value of this property as a string.
 */
void epayment_product::set_property(QString const & name, QString const & value)
{
    if(name == get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT))
    {
        throw epayment_cannot_set_exception("this property cannot be changed in an epayment_product object");
    }

    value_t v(value);

    f_properties[name] = v;
}


/** \brief Set a property as an integer.
 *
 * This function sets the named property using the specified integer as
 * the value.
 *
 * TBD -- shall we enforce the type of the property depending on its name?
 *
 * \exception epayment_cannot_set_exception
 * This function raises this exception if the property being set is
 * the product path which cannot be changed.
 *
 * \param[in] name  The name of the property to set to \p value.
 * \param[in] value  The value of this property as an integer.
 */
void epayment_product::set_property(QString const & name, int64_t const value)
{
    if(name == get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT))
    {
        throw epayment_cannot_set_exception("this property cannot be changed in an epayment_product object");
    }

    value_t v(value);
    f_properties[name] = v;
}


/** \brief Set a property as a floating point.
 *
 * This function sets the named property using the specified floating point as
 * the value.
 *
 * TBD -- shall we enforce the type of the property depending on its name?
 *
 * \exception epayment_cannot_set_exception
 * This function raises this exception if the property being set is
 * the product path which cannot be changed.
 *
 * \param[in] name  The name of the property to set to \p value.
 * \param[in] value  The value of this property as a floating point.
 */
void epayment_product::set_property(QString const & name, double const value)
{
    if(name == get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT))
    {
        throw epayment_cannot_set_exception("this property cannot be changed in an epayment_product object");
    }

    value_t v(value);
    f_properties[name] = v;
}


/** \brief Remove a property from the list.
 *
 * This function removes a property that was previously defined with a
 * set_property() call. Note that the product and quantity properties
 * cannot be unset.
 *
 * \exception epayment_cannot_unset_exception
 * This function raises this exception if the property being unset is
 * the product path, the quantity, or the description.
 *
 * \param[in] name  The name of the property to be unset.
 */
void epayment_product::unset_property(QString const & name)
{
    if(name == get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT)
    || name == get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY)
    || name == get_name(name_t::SNAP_NAME_EPAYMENT_DESCRIPTION))
    {
        throw epayment_cannot_unset_exception(QString("property \"%1\" cannot be unset from an epayment_product object").arg(name));
    }

    auto pos(f_properties.find(name));
    if(pos != f_properties.end())
    {
        f_properties.erase(pos);
    }
}


/** \brief Check whether a property was defined.
 *
 * This function goes through the list of properties for an object and
 * determines whether it was defined. If so, then the function returns
 * true.
 *
 * \note
 * It is very important to be noted: some properties are read from the
 * database: the product itself, the cart information, or an invoice.
 * Various functions know how to retrieve such parameters automatically,
 * although it makes use of a signal to obtain properties the epayment
 * plugin does not itself handle. This can make the function somewhat
 * slow, although it will save that property in the epayment_product
 * object for any future access.
 *
 * \todo
 * Implement various signals so other plugins have a chance to implement
 * similar capabilities.
 *
 * \param[in] name  The name of the property being checked.
 *
 * \return true if the property was defined with a set_property() call.
 */
bool epayment_product::has_property(QString const & name) const
{
    if(f_properties.find(name) != f_properties.end())
    {
        return true;
    }

    if(name == get_name(name_t::SNAP_NAME_EPAYMENT_RECURRING))
    {
        if(verify_guid())
        {
            // this is a GUID, try to get the info from the product
            // page first, if that fails, we will use a default below
            QString const recurring(f_revision_row->getCell(name)->getValue().stringValue());
            if(!recurring.isEmpty())
            {
                value_t r(recurring);
                f_properties[name] = r;
                return true;
            }
        }
        // it does not exist, the default is a null recurring entry
        return false;
    }

    // the property is not yet defined, check for some parameters that
    // the epayment system knows how to handle
    return name == get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY_MINIMUM)
        || name == get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY_MAXIMUM)
        || name == get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY_INCREMENT);
}


/** \brief Get the type of a property.
 *
 * In most cases the type of a property is known by the implementer to
 * be a string, an integer, or a floating point and thus which of
 * the get_..._property() functions to use is know at compile time.
 *
 * However, in some cases the type of a value may vary. In that case,
 * you certainly want to use this function to first determine which
 * type to use.
 *
 * \exception epayment_cannot_find_exception
 * The named property could not be found.
 *
 * \param[in] name  The name of the property to retrieve.
 *
 * \return The type of the property as one of the type enumerations values.
 */
epayment_product::type_t epayment_product::get_property_type(QString const & name) const
{
    auto const pos(f_properties.find(name));
    if(pos == f_properties.end())
    {
        throw epayment_cannot_find_exception("specified product property does not exist in this product");
    }
    return pos->second.get_type();
}


/** \brief Get the string of a property.
 *
 * This function retrieves the value of a string property. If the property
 * is not a string property, then the function will raise an exception.
 *
 * Special cases:
 *
 * \li epayment::recurring -- read this information from the product page
 *                            if not yet defined in this product
 *
 * \exception epayment_cannot_find_exception
 * The named property could not be found.
 *
 * \param[in] name  The name of the property to retrieve.
 *
 * \return The string value as such.
 *
 * \sa value_t::get_string_value()
 */
QString const epayment_product::get_string_property(QString const & name) const
{
    auto const pos(f_properties.find(name));
    if(pos == f_properties.end())
    {
        throw epayment_cannot_find_exception(QString("specified product property \"%1\" does not exist in this product").arg(name));
    }
    return pos->second.get_string_value(name);
}


/** \brief Get the integer of a property.
 *
 * This function retrieves the value of an integer property. If the property
 * is not an integer property, then the function will raise an exception.
 *
 * \exception epayment_cannot_find_exception
 * The named property could not be found.
 *
 * \param[in] name  The name of the property to retrieve.
 *
 * \return The integer value as such.
 *
 * \sa value_t::get_integer_value()
 */
int64_t epayment_product::get_integer_property(QString const& name) const
{
    auto const pos(f_properties.find(name));
    if(pos == f_properties.end())
    {
        throw epayment_cannot_find_exception("specified product property does not exist in this product");
    }
    return pos->second.get_integer_value();
}


/** \brief Get the floating point of a property.
 *
 * This function retrieves the value of a floating point property. If the
 * property is not a floating point property, then the function will
 * raise an exception.
 *
 * \exception epayment_cannot_find_exception
 * The named property could not be found.
 *
 * \param[in] name  The name of the property to retrieve.
 *
 * \return The floating point value as such.
 *
 * \sa value_t::get_float_value()
 */
double epayment_product::get_float_property(QString const & name) const
{
    auto const pos(f_properties.find(name));
    if(pos == f_properties.end())
    {
        if(name == get_name(name_t::SNAP_NAME_EPAYMENT_PRICE)
        || name == get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY_MINIMUM)
        || name == get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY_MAXIMUM)
        || name == get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY_INCREMENT))
        {
            if(verify_guid())
            {
                // this is a GUID, try to get the info from the product
                // page first, if that fails, we will use a default below
                libdbproxy::value value(f_revision_row->getCell(name)->getValue());
                if(value.size() == sizeof(double))
                {
                    double floating_point(value.doubleValue());
                    value_t v(floating_point);
                    f_properties[name] = v;
                    return floating_point;
                }
            }
            if(name == get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY_MINIMUM))
            {
                return 1.0;
            }
            if(name == get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY_MAXIMUM))
            {
                // TBD: should we use a max. such as 10,000 or something
                //      more reasonable than +oo?
                return std::numeric_limits<double>::infinity();
            }
            if(name == get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY_INCREMENT))
            {
                return 1.0;
            }
            // there is no default price...
        }
        throw epayment_cannot_find_exception("specified product property does not exist in this product");
    }
    return pos->second.get_float_value();
}


/** \brief Retrieve the total cost for this product.
 *
 * This function computes the total cost of this product. This includes
 * the product price times quantity. In some circumstances it may
 * include other costs such as taxes and shipping (although most often
 * this is computed as seperate products.)
 *
 * \return The total cost of this product.
 */
double epayment_product::get_total() const
{
    double const price(get_float_property(get_name(name_t::SNAP_NAME_EPAYMENT_PRICE)));
    double const quantity(get_float_property(get_name(name_t::SNAP_NAME_EPAYMENT_QUANTITY)));
    return price * quantity;
}






/** \brief Add a product to the list of products.
 *
 * This function adds a product to this list of products and returns a
 * reference to the caller. The reference will remain valid until the
 * next product gets added. You can get a new reference to the product
 * using the [] operator.
 *
 * \param[in] product  The GUID of the product.
 * \param[in] quantity  The quantity being purchased.
 * \param[in] description  The short description of the product.
 *
 * \return A reference to the new item.
 */
epayment_product& epayment_product_list::add_product(QString const & product, double const quantity, QString const & description)
{
    epayment_product p(product, quantity, description);
    f_products.push_back(p);
    return f_products.back();
}


/** \brief Check whether the list of products is empty.
 *
 * This function returns true if the list of products is empty (i.e. has
 * no products added yet, or was just cleared.)
 *
 * \return true if the list of products is currently empty.
 */
bool epayment_product_list::empty() const
{
    return f_products.empty();
}


/** \brief Retrieve the size of this list.
 *
 * This function retrieves the size of the list of products.
 *
 * \return The number of items in the list of products.
 */
size_t epayment_product_list::size() const
{
    return f_products.size();
}


/** \brief Clear this list of products.
 *
 * This function gets rid of all the products in this list. You can then
 * start fresh adding new products to the list.
 */
void epayment_product_list::clear()
{
    f_products.clear();
}


/** \brief Compute the grand total amount of this product list.
 *
 * This function calls the get_total() function of each of the
 * products in this list of products and sum them. It then returns
 * the result.
 *
 * Note that the system does not verify whether the grand total is
 * negative. If you have offers that can cover a large amount than
 * what the product(s) cost, then the grand total could be negative.
 *
 * \return The grand total (cost) of purchasing all those products.
 */
double epayment_product_list::get_grand_total() const
{
    double grand_total(0.0);
    size_t const max(f_products.size());
    for(size_t idx(0); idx < max; ++idx)
    {
        grand_total += f_products[idx].get_total();
    }
    return grand_total;
}


/** \brief Retrieve a reference to the specified product.
 *
 * This function returns a read/write reference to the specified product.
 * The \p idx parameter must be between 0 and size() - 1, so if empty()
 * returns true, this function cannot be called.
 *
 * The reference remains valid until the list changes (add_product() or
 * clear() gets called.)
 *
 * \param[in] idx  The index of the parameter to retrieve.
 *
 * \return A writable reference to the specified product.
 */
epayment_product& epayment_product_list::operator [] (int idx)
{
    return f_products[idx];
}


/** \brief Retrieve a reference to the specified product.
 *
 * This function returns a read-only reference to the specified product.
 * The \p idx parameter must be between 0 and size() - 1, so if empty()
 * returns true, this function cannot be called.
 *
 * The reference remains valid until the list changes (add_product() or
 * clear() gets called.)
 *
 * \param[in] idx  The index of the parameter to retrieve.
 *
 * \return A read-only reference to the specified product.
 */
epayment_product const& epayment_product_list::operator [] (int idx) const
{
    return f_products[idx];
}













/** \brief Create a default recurring object.
 *
 * This constructor creates a default recurring object which is an
 * infinite recurring object that charges the user once a month:
 *
 * \code
 *      1/MONTH
 * \endcode
 */
recurring_t::recurring_t()
    : f_repeat(INFINITE_REPEAT)
    , f_interval(1)
    , f_frequency(FREQUENCY_MONTH)
{
}


/** \brief Create a recurring object from the specified field.
 *
 * This constructor defines a default recurring object and then
 * parses the specified string to further initialize the object.
 *
 * \param[in] field  A valid recurring string.
 *
 * \sa set()
 */
recurring_t::recurring_t(QString const & field)
    : f_repeat(INFINITE_REPEAT)
    , f_interval(1)
    , f_frequency(FREQUENCY_MONTH)
{
    set(field);
}


/** \brief Set the recurring object fields as per the specified string.
 *
 * This function parses the specified string and saves the values as
 * expected in the various fields of the recurring object.
 *
 * The syntax is as follow:
 *
 * \code
 *      <repeat> 'x' <interval> '/' <frequency>
 * \endcode
 *
 * The \<repeat> parameter defines how many times the fee will be charged.
 * It is optional, if not specified then the system views the \<repeat>
 * as infinite.
 *
 * The \<interval> parameter defines the number of \<frequency> to wait
 * before processing a new charge. So if \<interval> is set to 3 and
 * \<frequency> is set to MONTH, a new charge is made once every quarter.
 * The \<interval> is optional if no \<repeat> is defined. It is required
 * otherwise. The default is 1 when not specified.
 *
 * The \<frequency> is one of DAY, WEEK, MONTH, YEAR. You may use additional
 * definitions in your system, although everything else (quarters, bimensual,
 * etc.) can generally be obtained with these 4 frequencies. The \<frequency>
 * is optional. The default is MONTH when not specified.
 *
 * \note
 * The \<repeat> and \<interval> numbers cannot be negative nor zero.
 *
 * \note
 * The function respect the contract. If the exception is raised, then the
 * current data of this recurring_t object is left unchanged.
 *
 * \exception epayment_invalid_recurring_field_exception
 * If the parser fails reading the entire field, this exception is raised.
 * Note that only the computer should generate those strings so there is
 * really no reason to have one invalid unless a programmer wrote one by
 * hand in which case he certainly wants to immediately know that it is
 * wrong.
 *
 * \param[in] field  The string representing the recurring information.
 */
void recurring_t::set(QString const & field)
{
    // create defaults
    repeat_t new_repeat(INFINITE_REPEAT);
    interval_t new_interval(1);
    frequency_t new_frequency(FREQUENCY_MONTH);

    QChar const *s(field.data());
    int number(0);
    for(; *s >= '0' && *s <= '9'; ++s)
    {
        number = number * 10 + s->unicode() - '0';
    }
    if(*s == 'x')
    {
        if(number == 0)
        {
            throw epayment_invalid_recurring_field_exception("recurring field: found 'x' without a number preceeding it (or just one or more zeroes)");
        }
        new_repeat = number;
        new_interval = 0;
        for(++s; *s >= '0' && *s <= '9'; ++s)
        {
            new_interval = new_interval * 10 + s->unicode() - '0';
        }
        if(new_interval == 0)
        {
            // fix up the object in case of an invalid number
            throw epayment_invalid_recurring_field_exception("recurring field: found 'x' without a number following it (<interval> is mandatory in this case)");
        }
    }
    else if(number != 0)
    {
        new_interval = number;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
    if(*s == '/' || number == 0)
    {
#pragma GCC diagnostic pop
        if(*s == '/')
        {
            ++s;
        }
        QChar const *e(s);
        for(; *e != '\0'; ++e);
        QString freq;
        freq.setRawData(s, e - s);
        // check in an order with the most likely frequency first
        if(freq == "MONTH")
        {
            new_frequency = FREQUENCY_MONTH;
        }
        else if(freq == "DAY")
        {
            new_frequency = FREQUENCY_DAY;
        }
        else if(freq == "WEEK")
        {
            new_frequency = FREQUENCY_WEEK;
        }
        else if(freq == "YEAR")
        {
            new_frequency = FREQUENCY_YEAR;
        }
        else if(freq == "TWICE_A_MONTH")
        {
            new_frequency = FREQUENCY_TWICE_A_MONTH;
        }
        else
        {
            throw epayment_invalid_recurring_field_exception(QString("recurring field: unknown frequence \"%1\".").arg(freq));
        }
    }
    else if(*s != '\0')
    {
        QChar const *e(s);
        for(; *e != '\0'; ++e);
        QString left_over;
        left_over.setRawData(s, e - s);
        throw epayment_invalid_recurring_field_exception(QString("recurring field: unknown characters (%1).").arg(left_over));
    }

    // string was all good, save the results
    // (the numbers are checked again... and we can still throw)
    set(new_repeat, new_interval, new_frequency);
}


/** \brief Setup a new recurring object.
 *
 * This function can be sued to setup a new recurring repeat, interval
 * and frequency. If you have a string, you may want to call the set()
 * function directly with that string.
 *
 * To reset the recurring_t object to the defaults, use the following:
 *
 * \code
 *      recurring.set(epayment::recurring_t::INFINITE_REPEAT,
 *                    1,
 *                    epayment::recurring_t::FREQUENCY_MONTH);
 * \endcode
 *
 * To reset the object to internal defaults, you can copy the values
 * using a new recurring object as in:
 *
 * \code
 *      recurring = epayment::recurring_t();
 * \endcode
 *
 * \exception epayment_invalid_recurring_field_exception
 * The function verifies that the \p repeat is between 1 and MAX_REPEAT
 * or equal to INFINITE_REPEAT. If not, the function raises this exception.
 * Further, the function makes sure that \p frequency is a valid frequency_t
 * enumeration value, if not, then this exception is raised. Then it verifies
 * that the \p interval / \p frequency duration is about MAX_INTERVAL_YEARS.
 * If larger, zero, or negative, then this exception is raised. Note that
 * the FREQUENCY_TWICE_A_MONTH (1st and 15th of the month) is a special
 * case and it cannot be used with an interval other than 1.
 *
 * \param[in] repeat  The number of time it should be repeated.
 * \param[in] interval  The number of days, weeks, months, years to wait
 *                      before the next charge.
 * \param[in] frequence  One of the frequency_t enumeration values.
 */
void recurring_t::set(repeat_t repeat, interval_t interval, frequency_t frequency)
{
    if((repeat < 0 && repeat != INFINITE_REPEAT)
    || repeat == 0
    || repeat > MAX_REPEAT)
    {
        throw epayment_invalid_recurring_field_exception("recurring field: repeat cannot be negative, null, or more than 1000.");
    }

    interval_t max_interval(0);
    switch(frequency)
    {
    case FREQUENCY_DAY:
        max_interval = MAX_INTERVAL_YEARS * 366;
        break;

    case FREQUENCY_WEEK:
        max_interval = MAX_INTERVAL_YEARS * 52;
        break;

    case FREQUENCY_TWICE_A_MONTH:
        max_interval = 1;
        break;

    case FREQUENCY_MONTH:
        max_interval = MAX_INTERVAL_YEARS * 12;
        break;

    case FREQUENCY_YEAR:
        max_interval = MAX_INTERVAL_YEARS;
        break;

    default:
        throw epayment_invalid_recurring_field_exception(QString("recurring: unknown frequency (%1).").arg(static_cast<int>(frequency)));

    }

    if(interval > max_interval)
    {
        // the maximum interval depends on the frequency
        throw epayment_invalid_recurring_field_exception(QString("recurring field: interval cannot be more than %1 with that frequency (%2).")
                    .arg(max_interval)
                    .arg(static_cast<int>(frequency)));
    }

    // Various further canonicalizations
    // 12/MONTH == 1/YEAR, 24/MONTH == 2/YEAR, etc.
    if(interval % 12 == 0 && frequency == FREQUENCY_MONTH)
    {
        interval /= 12;
        frequency = FREQUENCY_YEAR;
    }
    // TODO: do some more canonicalization of recurring_t frequencies

    f_repeat = repeat;
    f_interval = interval;
    f_frequency = frequency;
}


/** \brief Define the recurring data from a compressed value.
 *
 * To save the recurring_t object in a database, we generally use the
 * compressed format so that way it is very much smaller than than what
 * is generally achieved with the to_string() function.
 *
 * This function extracts the 3 fields from the compressed_t value.
 * The values still need to be valid as expected by the other set()
 * functions.
 *
 * \param[in] compressed  The compressed recurring_t values.
 */
void recurring_t::set(compressed_t compressed)
{
    repeat_t repeat((compressed >> REPEAT_SHIFT) & REPEAT_MASK);
    if(repeat == 0)
    {
        // when compressed the infinite repeat is saved as zero
        repeat = INFINITE_REPEAT;
    }
    set(repeat,
        (compressed >> INTERVAL_SHIFT) & INTERVAL_MASK,
        (compressed >> FREQUENCY_SHIFT) & FREQUENCY_MASK);
}


/** \brief Get the repeat.
 *
 * This function returns the repeat counter of this recurring object.
 *
 * The default is INFINITE_REPEAT. This value cannot be zero or
 * negative (outside of INFINITE_REPEAT).
 *
 * \return The repeat value. It may be INFINITE_REPEAT.
 */
recurring_t::repeat_t recurring_t::get_repeat() const
{
    return f_repeat;
}


/** \brief Get the interval.
 *
 * This function returns the interval counter of this recurring object.
 *
 * The interval represents the number of times the frequency is to be
 * multiplied to determine the dates of the following payments. So if
 * the interval is set to 5 and the frequency to WEEK, the payments
 * will be processed once every 5 weeks.
 *
 * The interval cannot be zero or negative. It is limited to 1 for
 * the FREQUENCY_TWICE_A_MONTH frequency. It is limitied to about
 * 5 years for other frequencies.
 *
 * \return The interval value.
 */
recurring_t::interval_t recurring_t::get_interval() const
{
    return f_interval;
}


/** \brief Get the frequency.
 *
 * The frequency is one of the frequency_t values:
 *
 * \li FREQUENCY_DAY -- the interval is defined in days.
 * \li FREQUENCY_WEEK -- the interval is defined in weeks.
 * \li FREQUENCY_TWICE_A_MONTH -- the interval must be 1; charge on the 1st
 *                                and the 15th of the month.
 * \li FREQUENCY_MONTH -- the interval is defined in months.
 * \li FREQUENCY_YEAR -- the interval is defined in years.
 *
 * \note
 * We may use of a frequency instead of just a number of days, because our
 * calendars are quite messed up. 1/MONTH does not represent any specific
 * number of days. It could be 27, 28, 30, or 31 days. Similarly, 1/YEAR
 * represents 365 or 366 days. Also some systems may want to charge all
 * customers on the first of the month instead of day the person registered.
 *
 * \return The frequency of this recurring object.
 */
recurring_t::frequency_t recurring_t::get_frequency() const
{
    return f_frequency;
}


/** \brief Transform this recurring object to a string.
 *
 * This function outputs the recurring object in the form of a string
 * which is useful to share in various environments such as JavaScript.
 * This is the opposite of the set() function using a string.
 *
 * The function tries to optimize the string whenever possible. There is
 * one exception: in case of "1/MONTH", that specific string is returned
 * instead of the empty string (since that represents 100% the default.)
 *
 * The optimization may be just the frequency: "DAY" means infinite
 * repeat and interval of 1.
 *
 * Finally, the optimization may be just the interval: "5" meaning infinite
 * repeat, charge once every 5 months.
 *
 * The defaults are:
 *
 * \li For repeat: INIFITE_REPEAT
 * \li For interval: 1
 * \li For frequency: MONTH
 *
 * Note that if a repeat is specified, then the interval becomes mandatory,
 * so "3x1" cannot be optimized to "3x" (which is considered invalid.)
 *
 * \return The recurring object as a string.
 */
QString recurring_t::to_string() const
{
    QString result;

    if(f_repeat != INFINITE_REPEAT)
    {
        result += QString("%1x").arg(f_repeat);
    }

    // frequency to string (we could have a separate static function for this one)
    QString const freq(freq_to_string(f_frequency));

    if(f_interval == 1 && result.isEmpty())
    {
        if(f_frequency == FREQUENCY_MONTH)
        {
            // a special case were users generally expect to see this...
            return "1/MONTH";
        }
        // Frequency is enough
        return freq;
    }

    result += QString("%1").arg(f_interval);

    if(f_frequency != FREQUENCY_MONTH)
    {
        result += QString("/%1").arg(freq);
    }

    return result;
}


/** \brief Compress the recurring object in one compressed_t integer.
 *
 * This function takes the current repeat, interval and frequency
 * and saves those in an integer.
 */
recurring_t::compressed_t recurring_t::to_compressed() const
{
    // we do not apply the masks here because the number of bits was
    // chosen carefully to work in link with the kind of numbers
    // we use in the recurring_t object
    //
    // save infinite repeat as 0 instead of -1
    return (f_repeat == INFINITE_REPEAT ? 0 : f_repeat << REPEAT_SHIFT)
         | (f_interval << INTERVAL_SHIFT)
         | (f_frequency << FREQUENCY_SHIFT);
}


/** \brief Transform the frequency into a string.
 *
 * This static function transforms the specified frequency enumeration
 * into a string.
 *
 * \return The frequency as a string.
 */
QString recurring_t::freq_to_string(frequency_t frequency)
{
    switch(frequency)
    {
    case FREQUENCY_DAY:
        return "DAY";

    case FREQUENCY_WEEK:
        return "WEEK";

    case FREQUENCY_TWICE_A_MONTH:
        return "TWICE_A_MONTH";

    case FREQUENCY_MONTH:
        return "MONTH";

    case FREQUENCY_YEAR:
        return "YEAR";

    default:
        throw epayment_invalid_recurring_field_exception(QString("freq_to_string(): unknown frequency (%1).").arg(static_cast<int>(frequency)));

    }
    NOTREACHED();
}


/** \brief Check whether this recurring object represents a null object.
 *
 * A recurring object with a repeat of 1 is considered null, since it
 * really represents a one time fee (repeating 1x a payment is not
 * a subscription).
 *
 * Null recurring payments should be ignored and a straight sale processed
 * instead, hence this test.
 *
 * \return true if f_repeat is exactly 1, whatever the interval and frequency.
 */
bool recurring_t::is_null() const
{
    return f_repeat == 1;
}


/** \brief Check whether this recurring object is infinite.
 *
 * A recurring object with a repeat set to INFINITE_REPEAT will repeat
 * until canceled. This function returns true if the repeat is infinite.
 *
 * \return true if f_repeat is INFINITE_REPEAT.
 */
bool recurring_t::is_infinite() const
{
    return f_repeat == INFINITE_REPEAT;
}


/** \brief Compare two recurring_t objects for equality.
 *
 * Since recurring_t objects are considered canonicalized once
 * a set() function was called, this function simply makes sure
 * that the repeat, interval, and frequency values are the
 * same in both objects.
 *
 * \param[in] rhs  The other recurring_t object to test against.
 *
 * \return true if both recurring_t objects represent the same frequency.
 */
bool recurring_t::operator == (recurring_t const & rhs) const
{
    return f_repeat == rhs.f_repeat
        && f_interval == rhs.f_interval
        && f_frequency == rhs.f_frequency;
}


/** \brief Compare two recurring_t objects for inequality.
 *
 * Since recurring_t objects are considered canonicalized once
 * a set() function was called, this function simply checks whether
 * at least one of the repeat, interval, and frequency values are
 * different in both objects.
 *
 * \param[in] rhs  The other recurring_t object to test against.
 *
 * \return true if both recurring_t objects represent different frequencies.
 */
bool recurring_t::operator != (recurring_t const & rhs) const
{
    return f_repeat != rhs.f_repeat
        || f_interval != rhs.f_interval
        || f_frequency != rhs.f_frequency;
}













/** \brief Initialize the epayment plugin.
 *
 * This function is used to initialize the epayment plugin object.
 */
epayment::epayment()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the epayment plugin.
 *
 * Ensure the epayment object is clean before it is gone.
 */
epayment::~epayment()
{
}


/** \brief Get a pointer to the epayment plugin.
 *
 * This function returns an instance pointer to the epayment plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the epayment plugin.
 */
epayment * epayment::instance()
{
    return g_plugin_epayment_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString epayment::settings_path() const
{
    return "/admin/settings/epayment";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString epayment::icon() const
{
    return "/images/epayment/epayment-logo-64x64.png";
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString epayment::description() const
{
    return "The e-Payment plugin offers one common way to process an"
          " electronic or not so electronic payment online (i.e. you"
          " may accept checks, for example...)";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString epayment::dependencies() const
{
    return "|content|editor|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t epayment::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2017, 4, 28, 19, 32, 45, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void epayment::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the epayment.
 *
 * This function terminates the initialization of the epayment plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void epayment::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(epayment, "layout", layout::layout, generate_header_content, _1, _2, _3);
}


/** \brief Setup page for the editor.
 *
 * The editor has a set of dynamic parameters that the users are offered
 * to setup. These parameters need to be sent to the user and we use this
 * function for that purpose.
 *
 * \todo
 * Look for a way to generate the editor data only if necessary (too
 * complex for now.)
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 */
void epayment::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(ipath);
    NOTUSED(metadata);

    QDomDocument doc(header.ownerDocument());

    // make sure this is a product, if so, add product fields
    links::link_info product_info(content::get_name(content::name_t::SNAP_NAME_CONTENT_PAGE_TYPE), true, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(product_info));
    links::link_info product_child_info;
    if(link_ctxt->next_link(product_child_info))
    {
        // the link_info returns a full key with domain name
        // use a path_info_t to retrieve the cpath instead
        content::path_info_t type_ipath;
        type_ipath.set_path(product_child_info.key());
        if(type_ipath.get_cpath().startsWith(get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH)))
        {
            // if the content is the main page then define the titles and body here
            FIELD_SEARCH
                (content::field_search::command_t::COMMAND_MODE, content::field_search::mode_t::SEARCH_MODE_EACH)
                (content::field_search::command_t::COMMAND_ELEMENT, metadata)
                (content::field_search::command_t::COMMAND_PATH_INFO_REVISION, ipath)

                // /snap/head/metadata/epayment
                (content::field_search::command_t::COMMAND_CHILD_ELEMENT, "epayment")

                // /snap/head/metadata/epayment/product-name
                (content::field_search::command_t::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_EPAYMENT_PRODUCT_NAME))
                (content::field_search::command_t::COMMAND_SELF)
                (content::field_search::command_t::COMMAND_SAVE_XML, "product-name")

                // /snap/head/metadata/epayment/product-description
                (content::field_search::command_t::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_EPAYMENT_DESCRIPTION))
                (content::field_search::command_t::COMMAND_SELF)
                (content::field_search::command_t::COMMAND_IF_FOUND, 1)
                    // use page title as a fallback
                    (content::field_search::command_t::COMMAND_FIELD_NAME, content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))
                    (content::field_search::command_t::COMMAND_SELF)
                (content::field_search::command_t::COMMAND_LABEL, 1)
                (content::field_search::command_t::COMMAND_SAVE_XML, "product-description")

                // /snap/head/metadata/epayment/product-price
                (content::field_search::command_t::COMMAND_FIELD_NAME, get_name(name_t::SNAP_NAME_EPAYMENT_PRICE))
                (content::field_search::command_t::COMMAND_SELF)
                (content::field_search::command_t::COMMAND_SAVE_FLOAT64, "product-price")

                // generate!
                ;
        }
    }

    // TODO: find a way to include e-Payment data only if required
    //       (it may already be done! search on add_javascript() for info.)
    content::content::instance()->add_javascript(doc, "epayment");
    content::content::instance()->add_css(doc, "epayment");
}


/** \brief Get an invoice status.
 *
 * This function reads the invoice status and returns it.
 *
 * A page that was never marked as an invoice will not have a status.
 * In that case the function returns
 * name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_UNKNOWN.
 *
 * When you create a page which represents an invoice, you should set
 * the invoice status to created as in:
 *
 * \code
 *    epayment::epayment::instance()->set_invoice_status(invoice_ipath,
 *           epayment::epayment::name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED);
 * \endcode
 *
 * It is VERY IMPORTANT to call the function since it is a signal and other
 * plugins may be listening to that signal and react accordingly.
 *
 * The statuses are defined here:
 *
 * \li name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_ABANDONED
 *            -- the payment failed too many times and the invoice was
 *               finally abandoned meaning that no more attempts to make
 *               a payment against that invoice shall happen
 * \li name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED
 *            -- the invoice was void in some ways; either the customer
 *               decided to not process the payment at all or the customer
 *               decided to cancel later in which case he was reimbursed;
 *               it could also be used when a payment is attempted too
 *               many times and fails each time (i.e. 3 attempts...)
 * \li name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED
 *            -- the invoice was paid and the shipping processed; this
 *               status is most often not used when there is no shipping
 *               (i.e. an online service); payment wise, COMPLETED also
 *               means that the products/services were PAID
 * \li name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED
 *            -- the invoice was just created; it is brand new and was
 *               not yet paid; it also means a payment was not attempted
 * \li name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED
 *            -- the customer attempted a payment and it failed; the
 *               customer is allowed to try again; however, auto-repeat
 *               is now turned off against that invoice
 * \li name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID
 *            -- the payment was received in full (we do not currently
 *               support partial payments, if you want to offer partial
 *               payments, you need to create multiple invoices)
 * \li name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING
 *            -- the payment request was sent to a processor and we are
 *               waiting for the reply by the processor; this status is
 *               not always used; (TBD: we probably should include a way
 *               to save the date when that started)
 * \li name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING
 *            -- the payment is being processed; this is generally used
 *               by processors that send users to an external website
 *               where they enter their information before doing their
 *               payment; this is different from pending because the
 *               customer has to act on it whereas pending means it is
 *               all automated
 * \li name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_UNKNOWN
 *            -- when checking a page with an epayment::status which is
 *               not one of the accepted statuses, this is returned
 *
 * \todo
 * As we extend functionality, we will add additional statuses. For
 * example, in order for a customer to get reimbursed, we may need
 * intermediate states similar to PROCESSING and PENDING, which
 * represent a state in wait of the reimbursement being worked on.
 *
 * \param[in] invoice_ipath  The path to the invoice.
 */
name_t epayment::get_invoice_status(content::path_info_t& invoice_ipath)
{
    content::content *content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    libdbproxy::row::pointer_t row(content_table->getRow(invoice_ipath.get_key()));
    QString const status(row->getCell(get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS))->getValue().stringValue());

    // convert string to ID, makes it easier to test the status
    if(status == get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_ABANDONED))
    {
        return name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_ABANDONED;
    }
    if(status == get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED))
    {
        return name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED;
    }
    if(status == get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED))
    {
        return name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED;
    }
    if(status == get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED))
    {
        return name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED;
    }
    if(status == get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED))
    {
        return name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED;
    }
    if(status == get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID))
    {
        return name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID;
    }
    if(status == get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING))
    {
        return name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING;
    }
    if(status == get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING))
    {
        return name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING;
    }

    return name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_UNKNOWN;
}


/** \brief Signal used to change the invoice status.
 *
 * Other plugins that want to react whenever an invoice changes its status
 * can make use of this signal. For example, once an invoice is marked PAID
 * and the cart included items that need to be shipped, the corresponding
 * plugin can make the invoice visible to the administrator who is
 * responsible for the handling.
 *
 * Another example is about users who purchase software. Once the invoice
 * is marked as PAID, the software becomes downloadable by the user.
 *
 * The list of invoice statuses is defined in the get_invoice_status()
 * function.
 *
 * \note
 * Although name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_UNKNOWN is considered a
 * possible status when you do a get_status(), you cannot actually set
 * an invoice to that status. If an invoice is somehow "lost", use the
 * canceled status instead: name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED.
 *
 * \todo
 * We need to see whether we want to enforce only legal status changes.
 * For example, a PAID invoice cannot all of a sudden be marked as
 * PENDING. At this point we let it go to see whether it should be
 * allowed to happen in some special situations.
 *
 * \exception snap_logic_exception
 * This exception is raised when the function is called with an invalid
 * status.
 *
 * \param[in,out] invoice_ipath  The path to the invoice changing its status.
 * \param[in] status  The new status as an epayment name_t.
 *
 * \return true if the status changed, false if the status does not change
 *         or an error is detected and we can continue.
 *
 * \sa get_invoice_status()
 */
bool epayment::set_invoice_status_impl(content::path_info_t& invoice_ipath, name_t const status)
{
    // make sure the status is properly defined
    switch(status)
    {
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_ABANDONED:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING:
        break;

    default:
        // status is contolled as the few types defined in this switch;
        // anything else is not allowed
        throw snap_logic_exception("invalid name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_...");

    }

    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());
    libdbproxy::row::pointer_t row(content_table->getRow(invoice_ipath.get_key()));
    QString const current_status(row->getCell(get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS))->getValue().stringValue());
    QString const new_status(get_name(status));
    if(current_status == new_status)
    {
        // status not changing, avoid any additional work
        return false;
    }
    row->getCell(get_name(name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS))->setValue(new_status);

    return true;
}


/** \brief Process a recurring payment.
 *
 * This function is used to process a recurring payment. The e-Payment
 * facility is not responsible (so far) in determining when a recurring
 * payment has to re-occur. This is the responsibility of the client.
 *
 * When a new invoice is created, call this signal with:
 *
 * \li The first invoice that was processed with a recurring payment
 *     (also called a subscription).
 * \li If there is one, provide the last payment that was made for that
 *     subscription. In most cases this is not required by the various
 *     payment facility. However, you cannot hope that the users of your
 *     code will never use a facility that requires that invoice so it
 *     has to be provided. If there is only one payment, this can be
 *     the same URL as first_invoice_ipath.
 * \li The new invoice you just created and want to furfill.
 *
 * The signal may fail if the charge happens either too soon or too late.
 * (Paypal checks the dates and prevents billing a recurring payment too
 * early on and their deadline date is not documented...)
 *
 * \param[in] first_invoice_ipath  The very first payment made for that
 *                                 recurring payment to be repeated.
 * \param[in] previous_invoice_ipath  The last invoice that was paid in
 *                                    this plan, or the same as
 *                                    first_invoice_ipath if no other
 *                                    invoices were paid since.
 * \param[in] new_invoice_ipath  The new invoice you just created.
 *
 * \return true if the 3 ipath references are considered valid to possibly
 *         generate a recurring payment.
 */
bool epayment::repeat_payment_impl(content::path_info_t & first_invoice_ipath, content::path_info_t & previous_invoice_ipath, content::path_info_t & new_invoice_ipath)
{
    content::content * content_plugin(content::content::instance());
    libdbproxy::table::pointer_t content_table(content_plugin->get_content_table());

    switch(get_invoice_status(new_invoice_ipath))
    {
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_ABANDONED:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED:
        // it was marked as paid or failed in some way so ignore the request
        SNAP_LOG_WARNING("repeat_payment() called with an invoice which is marked abandoned, canceled, paid, completed, or failed.");
        return false;

    default:
        // valid for auto-payment
        break;

    }

    switch(get_invoice_status(previous_invoice_ipath))
    {
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED:
        break;

    default:
        SNAP_LOG_WARNING("repeat_payment() called with a previous invoice not marked as paid or completed.");
        return false;

    }

    switch(get_invoice_status(first_invoice_ipath))
    {
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID:
    case name_t::SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED:
        break;

    default:
        SNAP_LOG_WARNING("repeat_payment() called with a first invoice not marked as paid or completed.");
        return false;

    }

    // valid so far, let the other modules take care of this repeat payment
    return true;
}


// List of bitcoin libraries and software
//   https://en.bitcoin.it/wiki/Software

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
