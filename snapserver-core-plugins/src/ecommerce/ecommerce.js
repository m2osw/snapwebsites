/** @preserve
 * Name: ecommerce
 * Version: 0.0.1.66
 * Browsers: all
 * Depends: editor (>= 0.0.3.262)
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
// @js plugins/editor/editor.js
// @js plugins/epayment/epayment.js
// ==/ClosureCompiler==
//

/*
 * JSLint options are defined online at:
 *    http://www.jshint.com/docs/options/
 */
/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false, FileReader: true, Blob: true */



/** \file
 * \brief The Snap! Website cart organization.
 *
 * This file defines a set of e-Commerce features using advance JavaScript
 * classes.
 *
 * This includes the Cart (the main object). You cannot create more than
 * one Cart. The Cart receives signals to add products to itself. These
 * products generally are referencing the ProductTypeBasic type. Other
 * types can be added. For example we are planning in offering a
 * ProductTypeKit which allows the e-Commerce administrator to create
 * bundles of products to be sold as one unit. The details of these
 * bundles should be shown in the cart and the specialized product type
 * is used for that purpose. It will likely be derived from the basic
 * product type.
 *
 * The resulting environment looks like this:
 *
 * \code
 *  +----------------------------+        +-----------------------+
 *  |                            |        |                       |
 *  | ServerAccessTimerCallbacks |    +-->| eCommerceColumnHeader |
 *  | (cannot instantiate)       |    |   | (header row)          |
 *  +----------------------------+    |   +-----------------------+
 *       ^                            |
 *       |                            |   +-----------------------+
 *       |               Create (1,n) |   |                       |
 *       |                            |   | eCommerceColumnCell   |
 *       | Inherit                    |   | (rows of products)    |
 *       |                            |   +-----------------------+
 *       |                            |          ^
 *       |                            |          | Create (1,n)
 *       |                            |          |
 *       |                            |          |
 *       |               +------------+----------+-----+
 *       |               |                             |    Access
 *       |  Create (1,n) | eCommerceColumns            |<--------------+
 *       |  +----------->|                             |               |
 *       |  |            +-----------------------------+               |
 *       |  |                                                          |
 *       |  |                       +-----------------------------+    |
 *       |  |                       |                             |    |
 *       |  |                       | eCommerceProductFeatureBase |    |
 *       |  |      +--------------->| (cannot instantiate)        |    |
 *       |  |      |                +-----------------------------+    |
 *       |  |      |                      ^                            |
 *       |  |      | Reference            | Inherit                    |
 *       |  |      |                      |                            |
 *  +----+--+------+----+           +-----+-----------------------+    |
 *  |                   |           |                             +----+
 *  | eCommerceCart     |<----------+ eCommerceProductFeature...  |
 *  | (final)           | Register  |                             |<--------+
 *  +-----+------+------+ (1,n)     +---------------------------+-+         |
 *    ^   |      |  ^                      ^                    |           |
 *    |   |      |  | Register             | Reference (1,n)    |           |
 *    |   |      |  | (1,n)                |                    |           |
 *    |   |      |  |       +--------------+------------+       |           |
 *    |   |      |  +-------+                           |       |           |
 *    |   |      |          | eCommerceProductType      |       |           |
 *    |   |      |          |                           |       |           |
 *    |   |      |          +---------------------------+       |           |
 *    |   |      |                         ^                    |           |
 *    |   |      | Create (1,n)            | Reference          |           |
 *    |   |      V                         |                    |           |
 *    |   |  +-------------------+         |                    |           |
 *    |   |  |                   +---------+             Access |           |
 *    |   |  | eCommerceProduct  |                              v           |
 *    |   |  |                   |             +----------------------+     |
 *    |   |  |                   +------------>|                      |     |
 *    |   |  +-------------------+     Inherit | eCommerceProductBase |     |
 *    |   |                                    |                      |     |
 *    |   |                                    +----------------------+     |
 *    |   | Create (1,1)                                                    |
 *    |   |      +-------------------+                                      |
 *    |   +----->|                   +--... (widgets, etc.)                 |
 *    |          | EditorForm        |                                      |
 *    |          |                   |                                      |
 *    |          +-------------------+                                      |
 *    |                                                                     |
 *    | Create (1,1)                                                        |
 *  +-+----------------------+  Create (1,n)                                |
 *  |                        +----------------------------------------------+
 *  |   jQuery()             |
 *  |   Initialization       |
 *  |                        |
 *  +------------------------+
 * \endcode
 *
 * Note that the cart is something that happens to always be around
 * by default since an e-Commerce site is willing to sell, sell, sell
 * and thus having the cart always at hand is the best way to make
 * sure that your users know how to add items in there and how to
 * checkout and send you tons of money.
 *
 * The cart uses the ServerAccess object capabilities to send cart
 * updates to the server. This allows us to save the cart in
 * Cassandra and avoid losing it. Not only that, this means the
 * user can close everything, come back a week later, and still
 * find all his information intact.
 *
 * The base ecommerce plugin supports a basic feature set with
 * its eCommerceProductFeatureBasic object. This object supports
 * the following in link with the cart table of products:
 *
 * \li Product designation (name, nomemclature)
 * \li Product price (Sales price)
 * \li Product quantity (no specific measurement support)
 * \li Product total costs (price x quantity)
 * \li Cart grand total (sum of all the total costs)
 * \li Cart quantity (sum of all product quantities)
 */



/** \brief Snap eCommerceColumnHeader constructor.
 *
 * The list of columns define a header and this class registers
 * the column name and display name for the column.
 *
 * \code
 *  final class eCommerceColumnHeader
 *  {
 *  public:
 *      eCommerceColumnHeader(name, display_name);
 *
 *      function getName() : string;
 *      function getDisplayName() : string;
 *
 *  private:
 *      var name_: string;
 *      var displayName_: string;
 *  };
 * \endcode
 *
 * @param {string} name  The technical name of the header.
 * @param {string} display_name  The translated display name of the header.
 *
 * @return {snapwebsites.eCommerceColumnHeader}
 * @constructor
 * @struct
 */
snapwebsites.eCommerceColumnHeader = function(name, display_name)
{
    this.name_ = name;
    this.displayName_ = display_name;

    return this;
};


/** \brief eCommerceColumnHeader is a base class.
 *
 * It is expected to be an internal class only. Only the eCommerceProduct
 * object is expected to create objects from this class.
 */
snapwebsites.base(snapwebsites.eCommerceColumnHeader);


/** \brief The technical name of this column.
 *
 * The columns are given a technical name which does not change with the
 * current language. This is that name.
 *
 * @type {string}
 * @private
 */
snapwebsites.eCommerceColumnHeader.prototype.name_ = "";


/** \brief The technical name of this column.
 *
 * The columns are given a technical name which does not change with the
 * current language. This is that name.
 *
 * @type {string}
 * @private
 */
snapwebsites.eCommerceColumnHeader.prototype.displayName_ = "";


/** \brief Add a column to the list of columns.
 *
 * This function is generally called by the various
 * eCommerceProductFeature implementations.
 *
 * @return {string}  The technical name.
 *
 * @final
 */
snapwebsites.eCommerceColumnHeader.prototype.getName = function()
{
    return this.name_;
};


/** \brief Add a column to the list of columns.
 *
 * This function is generally called by the various
 * eCommerceProductFeature implementations.
 *
 * @return {string}  The translated display name.
 *
 * @final
 */
snapwebsites.eCommerceColumnHeader.prototype.getDisplayName = function()
{
    return this.displayName_;
};



/** \brief Snap eCommerceColumnCell constructor.
 *
 * The list of columns define a header and values for each column.
 * The header is created first. That is used to create the content
 * of the following rows.
 *
 * \code
 *  final class eCommerceColumnCell
 *  {
 *  public:
 *      eCommerceColumnCell(column_name: String, value: String|Number);
 *
 *      function getName() : String;
 *      function getValue() : String|Number;
 *
 *  private:
 *      var name_: string;
 *      var value_: string;
 *  };
 * \endcode
 *
 * @param {!string} column_name  The name of the column where this cell belongs.
 * @param {string|number} value  The value of this cell.
 *
 * @return {snapwebsites.eCommerceColumnCell}
 * @constructor
 * @struct
 */
snapwebsites.eCommerceColumnCell = function(column_name, value)
{
    this.name_ = column_name;

    // we cannot use our case here because the value may not be a string
    this.value_ = /** @type string */ (value);

    return this;
};


/** \brief eCommerceColumnCell is a base class.
 *
 * It is expected to be an internal class only. Only the eCommerceColumns
 * object is expected to create objects from this class.
 */
snapwebsites.base(snapwebsites.eCommerceColumnCell);


/** \brief The technical name of this column.
 *
 * The columns are given a technical name which does not change with the
 * current language. This is that name.
 *
 * @type {string}
 * @private
 */
snapwebsites.eCommerceColumnCell.prototype.name_ = "";


/** \brief The value of this cell.
 *
 * This parameter holds the value of this cell. It is defined as a
 * string even though it probably should be an Object since any
 * one feature is pretty much free to define whatever it wants as
 * the value. However, ultimately we have to present the value to
 * the end user and thus we better have a string for that purpose.
 *
 * \todo
 * We may want to look into the possibility to use the .toString()
 * function of the Object instead of expecting a string being
 * returned. That would probably open more possibilities in the
 * future.
 *
 * @type {string}
 * @private
 */
snapwebsites.eCommerceColumnCell.prototype.value_ = "";


/** \brief Add a column to the list of columns.
 *
 * This function is generally called by the various
 * eCommerceProductFeature implementations.
 *
 * @return {string}  The technical name of the cell, which corresponds to
 *                   the column name.
 *
 * @final
 */
snapwebsites.eCommerceColumnCell.prototype.getName = function()
{
    return this.name_;
};


/** \brief Add a column to the list of columns.
 *
 * This function is generally called by the various
 * eCommerceProductFeature implementations.
 *
 * @return {string}  The value in this cell.
 *
 * @final
 */
snapwebsites.eCommerceColumnCell.prototype.getValue = function()
{
    return this.value_;
};



/** \brief Snap eCommerceColumns constructor.
 *
 * The cart is composed of products which compute columns to be displayed
 * in the cart.
 *
 * \code
 *  class eCommerceColumns
 *  {
 *  public:
 *      eCommerceColumns();
 *
 *      function size() : number;
 *      function addColumnHeader(column_name: String, display_name: String, before_column: String) : Void;
 *      function getColumnHeader(index: Number) : eCommerceColumnHeader;
 *      function generateColumnsMap() : Void;
 *      function getColumnIndex(name: String) : Number;
 *      function addColumnData(row_index: Number, column_name: String, value: String|Number) : Void;
 *      function getColumnData(row_index: Number, column_index: Number) : eCommerceColumnCell;
 *
 *  private:
 *      var columnHeaders_: Array;
 *      var columnMap_: Object; // column headers mapped by name
 *      var rows_: Array;
 *  };
 * \endcode
 *
 * @return {snapwebsites.eCommerceColumns}
 * @constructor
 * @struct
 */
snapwebsites.eCommerceColumns = function()
{
    this.columnHeaders_ = [];
    this.columnMap_ = {};
    this.rows_ = [];

    return this;
};


/** \brief eCommerceColumns is a base class.
 *
 * It is expected to be an internal class only. Only the eCommerceProduct
 * object is expected to create objects from this class.
 */
snapwebsites.base(snapwebsites.eCommerceColumns);


/** \brief The list of columns.
 *
 * The columns are objects that define the name of the column, the display
 * name of the column, and possibly other information (feature specific).
 *
 * @type {Array.<snapwebsites.eCommerceColumnHeader>}
 * @private
 */
snapwebsites.eCommerceColumns.prototype.columnHeaders_; // = []; -- initialized in constructor to avoid problems


/** \brief The list of column indices by name.
 *
 * The following is a map to quickly convert a column name to an index.
 *
 * @type {Object}
 * @private
 */
snapwebsites.eCommerceColumns.prototype.columnMap_; // = {}; -- initialized in constructor to avoid problems


/** \brief The array of rows.
 *
 * The columns are objects that define the name of the column, the display
 * name of the column, and possibly other information (feature specific).
 *
 * @type {Array.<Array.<snapwebsites.eCommerceColumnCell>>}
 * @private
 */
snapwebsites.eCommerceColumns.prototype.rows_; // = []; -- initialized in constructor to avoid problems


/** \brief Get the total number of columns in this object.
 *
 * The columns object registers a given set of column headers
 * and columns of data. That number is the same once the columns
 * object is setup. This function can be used to retrieve that
 * number.
 *
 * @throws {Error}  The count of the header and data arrays are not the
 *                  same. You cannot call this function before you are
 *                  done with adding all the headers and data.
 *
 * @return {number}  The number of columns in this columns object.
 */
snapwebsites.eCommerceColumns.prototype.size = function()
{
    var header_size = this.columnHeaders_.length;

//#ifdef DEBUG
    if(header_size != snapwebsites.mapSize(this.columnMap_))
    {
        throw new Error("The header and map length are not equal");
    }
    // The following is incorrect when we create rows with "missing" columns
    // because features do not provide a value for certain rows...
    //var max_rows = this.rows_.length, i;
    //for(i = 0; i < max_rows; ++i)
    //{
    //    if(header_size != this.rows_[i].length)
    //    {
    //        throw new Error("row #" + i + " does not have the same length (" + this.rows_[i].length + ") as the header (" + header_size + ").");
    //    }
    //}
//#endif

    return header_size;
};


/** \brief Add a column to the list of columns.
 *
 * This function is generally called by the various
 * eCommerceProductFeature implementations.
 *
 * @param {string} column_name  The name of the column.
 * @param {string} display_name  The display name of the column.
 * @param {string} before_column  The name of a column which should be
 *                                after the new column.
 */
snapwebsites.eCommerceColumns.prototype.addColumnHeader = function(column_name, display_name, before_column)
{
    var max = this.columnHeaders_.length,
        header = new snapwebsites.eCommerceColumnHeader(column_name, display_name),
        i;

    if(before_column)
    {
        for(i = 0; i < max; ++i)
        {
            if(before_column == this.columnHeaders_[i].getName())
            {
                this.columnHeaders_.splice(i, 0, header);
                return;
            }
        }
    }

    this.columnHeaders_.push(header);
};


/** \brief Add a column to the list of columns.
 *
 * This function is generally called by the various
 * eCommerceProductFeature implementations.
 *
 * @param {number} index  The index of the column to retrieve.
 *
 * @return {snapwebsites.eCommerceColumnHeader}  The header at the specified
 *                                               index.
 */
snapwebsites.eCommerceColumns.prototype.getColumnHeader = function(index)
{
    if(index < 0 || index >= this.columnHeaders_.length)
    {
        throw new Error("index out of bounds in snapwebsites.eCommerceColumns.getColumnHeader()");
    }

    return this.columnHeaders_[index];
};


/** \brief Generate a map to find column indices from their name.
 *
 * When adding column headers, we give each column a technical name.
 * Then when adding a row of cells, we are given that name again.
 * This function generates a map to quickly find column indices and
 * save their value quickly.
 */
snapwebsites.eCommerceColumns.prototype.generateColumnsMap = function()
{
    var max = this.columnHeaders_.length,
        name,
        i;

    for(i = 0; i < max; ++i)
    {
        name = this.columnHeaders_[i].getName();
        if(!isNaN(this.columnMap_[name]))
        {
            throw new Error("you defined two header columns with the same name \"" + name + "\"");
        }
        this.columnMap_[name] = i;
    }
};


/** \brief Generate a map to find column indices from their name.
 *
 * When adding column headers, we give each column a technical name.
 * Then when adding a row of cells, we are given that name again.
 * This function generates a map to quickly find column indices and
 * save their value quickly.
 *
 * @param {string} name  The technical name of the column.
 *
 * @return {number}  The column index.
 */
snapwebsites.eCommerceColumns.prototype.getColumnIndex = function(name)
{
//#ifdef DEBUG
    // mapSize() is slow, so we want this in DEBUG only
    if(this.columnHeaders_.length != snapwebsites.mapSize(this.columnMap_))
    {
        throw new Error("It looks like generateColumnsMap() was not called yet.");
    }
//#endif
    if(isNaN(this.columnMap_[name]))
    {
        throw new Error("header named \"" + name + "\" not found in snapwebsites.eCommerceColumns.getColumnIndex()");
    }
    return this.columnMap_[name];
};


/** \brief Add a column to the list of columns.
 *
 * This function is generally called by the various
 * eCommerceProductFeature implementations.
 *
 * Note that to replace the value of a cell, simply call this function
 * again with the same \p column_name parameter.
 *
 * @param {number} row_index  The name of the column.
 * @param {string} column_name  The display name of the column.
 * @param {string|number} value  The name of a column which should be
 *                               after the new column.
 */
snapwebsites.eCommerceColumns.prototype.addColumnData = function(row_index, column_name, value)
{
    var cell = new snapwebsites.eCommerceColumnCell(column_name, value),
        i = this.columnMap_[column_name];

    // note: the columns may not be added in order, that should not cause
    //       any problem; re-adding the same will overwrite the previous value
    if(!this.rows_[row_index])
    {
        this.rows_[row_index] = [];
    }
    this.rows_[row_index][i] = cell;
};


/** \brief Add a column to the list of columns.
 *
 * This function is generally called by the various
 * eCommerceProductFeature implementations.
 *
 * \warning
 * A column may be undefined. In which case this function returns
 * 'undefined' or 'null'.
 *
 * @param {number} row_index  The index of the row being read.
 * @param {number} column_index  The index of the column being read.
 *
 * @return {snapwebsites.eCommerceColumnCell}  The cell representing this
 *                                             data entry.
 */
snapwebsites.eCommerceColumns.prototype.getColumnData = function(row_index, column_index)
{
    var max = this.columnHeaders_.length;

    if(row_index < 0 || row_index >= this.rows_.length)
    {
        throw new Error("row_index out of bounds in snapwebsites.eCommerceColumns.getColumnData()");
    }

    // the Total should always be last, but I think this is incorrect
    // in the grand scheme of things (i.e. depending on the row setup
    // some column may be undefined and thus this test can fail on
    // an otherwise valid row)
    if(column_index < 0 || column_index >= max)
    {
        throw new Error("column_index out of bounds in snapwebsites.eCommerceColumns.getColumnHeader()");
    }

    return this.rows_[row_index][column_index];
};



/** \brief Snap eCommerceProductType constructor.
 *
 * Whenever the ecommerce plugin generates a product page, it adds a
 * little piece of code defining the type of that product. This includes
 * various things such as a list of features, the price to purchase that
 * item, if available, quantities, etc.
 *
 * This is created when the page loads, once per type of product offered
 * on the page. It is expected to connect to all the anchors that allow
 * the user to add the product to the cart.
 *
 * Note that if the user has a non-empty cart, you will also
 * find the list of product types that correspond to the products
 * in the cart.
 *
 * The product type does not really know how to handle its data except
 * one which defines the list of features this product understands/makes
 * use of. The features make use of the other data.
 *
 * Note that a product type does not get initialized (much) up until the
 * point when the client clicks on one of the Buy Product buttons. At
 * that point the 'eCommerceProductFeature...' objects will sure be ready.
 *
 * The product type objects are created internally whenever you call
 * the registerProductType() function of the cart. There is no default
 * product type.
 *
 * \code
 * snapwebsites.eCommerceCartInstance.registerProductType({...});
 * \endcode
 *
 * So that way we have a map of product types available under
 * snapwebsites.eCommerceProductType.productTypes.
 *
 * \code
 *  class eCommerceProductType
 *  {
 *  public:
 *      function eCommerceProductType(data: Object);
 *
 *      function getProperty(name: string) : Object;
 *
 *  private:
 *      data_: Object;  // map of product properties
 *  };
 * \endcode
 *
 * The data map is defined as follow for the Basic features:
 *
 * \li 'ecommerce::features' -- list of feature names, the name of each
 * feature that is attached to this product type appears in this list;
 * the names are comma separated, no spaces.
 * \li 'ecommerce::guid' -- the GUID of this product.
 * \li 'ecommerce::description' -- the name of the product, as it should appear
 * in the cart.
 * \li 'ecommerce::price' -- the purchase price of this product; later
 * we will offer a quantity based price too.
 * \li 'ecommerce::start_quantity' -- product quantity added to the cart
 * on creation, if undefined or less or equal to zero, use 1 as a fallback.
 * The Add function may also change that amount once the product was created.
 * \li 'ecommerce::add_quantity' -- product quantity when re-adding the
 * same item to the cart; this can be set to 0 to not re-add (let the user
 * edit the quantity manually instead); it could be set to any other positive
 * number; we can also use the string 'ask=\<count>' so we ask the user
 * where \<count> should be added to that item quantity.
 * \li 'ecommerce::maximum_count' -- the maximum number of items one can
 * add in the cart of this type of product; the product type defines this
 * maximum because in some cases we have counts that are per 1,000 or even
 * more so just using a number such as 100 would not work well for any type
 * of information. If undefined, then Snap uses 100.
 *
 * Other plugins can add features and thus add to this map at will.
 *
 * @param {Object} data  The map representing the data of this product type.
 *
 * @return {snapwebsites.eCommerceProductType} The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.eCommerceProductType = function(data)
{
    this.data_ = data;

    return this;
};


/** \brief Mark eCommerceProductType as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.eCommerceProductType);


/** \brief The data describing this product type.
 *
 * This parameter receives an object describing the product details
 * such as the product price, description, etc.
 *
 * See the constructor description for additional information.
 *
 * @type {Object}
 * @private
 */
snapwebsites.eCommerceProductType.prototype.data_ = null;


/** \brief Retrieve a property from this product type.
 *
 * This function is used to retrieve a property from the product type
 * data object.
 *
 * \note
 * The name of the property is expected to always include the
 * namespace of the plugin defining that property. For example,
 * the list of features definition is named: 'ecommerce::features'.
 *
 * @param {!string} name  The name of the property to retrieve.
 *
 * @return {Object}  The content of the named property.
 *                   May be null or undefined.
 */
snapwebsites.eCommerceProductType.prototype.getProperty = function(name)
{
    return this.data_[name];
};



/** \brief Snap eCommerceProductBase constructor.
 *
 * The cart is composed of products. The base of the product is an
 * interface defined to allow the product features to call functions
 * on the product objects.
 *
 * \code
 *  class eCommerceProductBase
 *  {
 *  public:
 *      eCommerceProductBase();
 *
 *      abstract function getDescription() : String;
 *      abstract function getQuantity() : Number;
 *      abstract function setQuantity(quantity: Number) : Void;
 *      abstract function getPrice() : Number;
 *      abstract function getColumns(index: Number, features: Array of eCommerceFeaturesBase, columns: eCommerceColumns) : Void;
 *      abstract function getFeatures(features: Array of eCommerceFeaturesBase, use_column_features: Boolean) : Void;
 *      abstract function getProductType() : eCommerceProductType;
 *  };
 * \endcode
 *
 * @return {snapwebsites.eCommerceProductBase}
 * @constructor
 * @struct
 */
snapwebsites.eCommerceProductBase = function()
{
    return this;
};


/** \brief eCommerceProductBase is a base class.
 *
 * It is expected to be an internal class only. Only the eCommerceProduct
 * object is expected to inherit from this class.
 */
snapwebsites.base(snapwebsites.eCommerceProductBase);


/*jslint unparam: true */
/** \brief Get the description of this product.
 *
 * This function is returns a string with the description of this product.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override.)
 *
 * @return {string}  The description of the product.
 */
snapwebsites.eCommerceProductBase.prototype.getDescription = function() // abstract
{
    throw new Error("snapwebsites.eCommerceProductBase.getDescription() does not do anything (yet)");
};
/*jslint unparam: false */


/** \brief Get the quantity of this product.
 *
 * This function is returns a number with the quantity of this product.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override.)
 *
 * @return {number}  The quantity of the product.
 */
snapwebsites.eCommerceProductBase.prototype.getQuantity = function() // abstract
{
    throw new Error("snapwebsites.eCommerceProductBase.getQuantity() is not implemented");
};


/*jslint unparam: true */
/** \brief Set a new quantity for this product.
 *
 * This function changes the quantity to the newly specified value.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override.)
 *
 * @param {number} quantity  The new quantity of the product.
 */
snapwebsites.eCommerceProductBase.prototype.setQuantity = function(quantity) // abstract
{
    throw new Error("snapwebsites.eCommerceProductBase.setQuantity() is not implemented");
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Get the price of this product.
 *
 * This function is returns a number with the price of this product.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override.)
 *
 * @return {number}  The price of the product.
 */
snapwebsites.eCommerceProductBase.prototype.getPrice = function() // abstract
{
    throw new Error("snapwebsites.eCommerceProductBase.getPrice() is not implemented");
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Get the columns of this product.
 *
 * This function is used to add data to each column of a product row.
 * The cart does that by going through the array.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override.)
 *
 * @param {number} index  The index of the row being created.
 * @param {Array.<snapwebsites.eCommerceProductFeatureBase>} features  A reference to an array of features.
 * @param {!snapwebsites.eCommerceColumns} columns  The price of the product.
 */
snapwebsites.eCommerceProductBase.prototype.getColumns = function(index, features, columns)
{
    throw new Error("snapwebsites.eCommerceProductBase.getColumns() is not implemented");
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Merge two arrays of column features.
 *
 * This function is used to merge the array of features passed in and
 * the array of features of this product.
 *
 * This is used to generate the array of features necessary to generate
 * the cart header. We may later want to find a better way to handle
 * this special case. One way would be to force all features to always
 * add all their column in all the products, but right now we do NOT
 * force a product to make use of all the features (i.e. one product
 * may have taxes, but no shipping, another may have neither, one
 * may have both... thus you may get 1 or 2 columns differences.)
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override.)
 *
 * @param {Array.<snapwebsites.eCommerceProductFeatureBase>} features  A reference to an array of features.
 * @param {boolean} use_column_features  If true, use the column features, if false, use the footer features.
 */
snapwebsites.eCommerceProductBase.prototype.getFeatures = function(features, use_column_features)
{
    throw new Error("snapwebsites.eCommerceProductBase.getFeatures() is not implemented");
};
/*jslint unparam: false */


/** \brief Get a reference to the product type.
 *
 * Whenever a product is added to the cart, it is given a reference to
 * a product type. This function returns a copy to that reference.
 *
 * \todo
 * We probably should move the product type reference to the base object.
 *
 * @return {snapwebsites.eCommerceProductType}  The product type of this product.
 */
snapwebsites.eCommerceProductBase.prototype.getProductType = function()
{
    throw new Error("snapwebsites.eCommerceProductBase.getProductType() was not overriden");
};



/** \brief Snap eCommerceProductFeatureBase constructor.
 *
 * The cart works with products and each one of these products has a
 * type. In most cases products are given the default type which is
 * ProductTypeBasic.
 *
 * To make product objects fully dynamic, we define a base class here
 * and let other programmers add new product types in their own .js
 * files by extending this class (or another core type.)
 *
 * Features must be registered with the registerProductFeature() function
 * as in:
 *
 * \code
 *    snapwebsites.eCommerceCartInstance.registerProductFeature(new my_product_feature());
 * \endcode
 *
 * This base class already implements a few things that are common to
 * all products.
 *
 * \code
 *  class eCommerceProductFeatureBase
 *  {
 *  public:
 *      eCommerceProductFeatureBase();
 *
 *      // do NOT call the base class functions marked 'abstract'
 *      abstract function getFeatureName() : String;
 *      abstract function setupColumnHeaders(columns: eCommerceColumns) : Void;
 *      abstract function setupColumnFooters(columns: eCommerceColumns, index: Number) : Void;
 *      abstract function setupColumns(product: eCommerceProductBase, columns: eCommerceColumns, index: Number) : void;
 *      abstract function generateFooter(columns: eCommerceColumns, result: Object, net_grand_total: Number) Void;
 *
 *      virtual function getColumnDependencies() : String;
 *      virtual function getFooterDependencies() : String;
 *  };
 * \endcode
 *
 * @constructor
 * @struct
 */
snapwebsites.eCommerceProductFeatureBase = function()
{
    // TBD
    // Maybe at some point we want to create yet another layer
    // so we can have an auto-register, but I am not totally sure
    // that would really work right in all cases...
    //snapwebsites.Cart.registerProductFeature(this);

    return this;
};


/** \brief eCommerceProductFeatureBase is a base class.
 *
 * Note that if you inherit from a product feature and implements these
 * functions, make sure to call the super version too, unless declared as
 * abstract, in case they do something that is required. At this point the
 * base class does nothing in those callbacks although we may add error
 * handling in the error callback.
 */
snapwebsites.base(snapwebsites.eCommerceProductFeatureBase);


/** \brief Retrieve the name of this product feature.
 *
 * This function returns the product feature name. It is used whenever
 * you register the feature in the Cart object.
 *
 * @throws {Error} The base feature function throws an error as it should
 *                 never get called (abstract functions require override.)
 *
 * @return {string}  The name of this cart product feature as a string.
 */
snapwebsites.eCommerceProductFeatureBase.prototype.getFeatureName = function() // virtual
{
    throw new Error("snapwebsites.eCommerceProductFeatureBase.getFeatureName() was not overloaded.");
};


/*jslint unparam: true */
/** \brief Setup the columns of the cart header.
 *
 * This function is called to setup the column names and other information.
 * It has to be called first so the data can be added to the right columns
 * (i.e. by referencing a technical name used when setting up the headers.)
 *
 * The basic feature takes care of a few default columns. Other features
 * may add more colunms. It can also delete a column if it wants to although
 * that is unusual, the cart is flexible enough for that purpose.
 *
 * @param {!snapwebsites.eCommerceColumns} columns  A set of cart columns.
 */
snapwebsites.eCommerceProductFeatureBase.prototype.setupColumnHeaders = function(columns)
{
    throw new Error("snapwebsites.eCommerceProductFeatureBase.setupColumnsHeaders() was not overloaded.");
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Setup the columns of the cart footer.
 *
 * This function is called to setup the last row of the cart with \em footer
 * information. In most cases, this includes the total number of items
 * and the total costs of all the items x quantity.
 *
 * The basic feature takes care of those two values by default. Other
 * features may want to also define values here. Although if a feature
 * allows decimal quantities using various units, it may want to either
 * remove (set to "") or fix the total quantity which otherwise does
 * not make much sense.
 *
 * @param {!snapwebsites.eCommerceColumns} columns  A set of cart columns.
 * @param {number} index  The row index for this product entry.
 */
snapwebsites.eCommerceProductFeatureBase.prototype.setupColumnFooters = function(columns, index) // abstract
{
    throw new Error("snapwebsites.eCommerceProductFeatureBase.setupColumnFooters() does not do anything (yet)");
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Initialize this product feature.
 *
 * This function is called whenever a snapwebsites.Product is
 * created. It is expected to initialize the product.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override.)
 *
 * @param {!snapwebsites.eCommerceProductBase} product  The product which columns are being initialized.
 * @param {!snapwebsites.eCommerceColumns} columns  The columns being added.
 * @param {number} index  The index of the row being created.
 */
snapwebsites.eCommerceProductFeatureBase.prototype.setupColumns = function(product, columns, index) // abstract
{
    throw new Error("snapwebsites.eCommerceProductFeatureBase.setupColumns() was not overridden.");
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Add the Grand Total row at the bottom of the cart.
 *
 * The basic feature adds the Grand Total row at the very bottom of the
 * cart. The footer features order ensures that the basic feature be
 * last.
 *
 * @param {!snapwebsites.eCommerceColumns} columns  The columns were we want to save the product data.
 * @param {Object} result  The result object.
 * @param {number} net_grand_total  The net grand total to purchase all the products in the cart.
 */
snapwebsites.eCommerceProductFeatureBase.prototype.generateFooter = function(columns, result, net_grand_total) // abstract
{
    throw new Error("snapwebsites.eCommerceProductFeatureBase.generateFooter() does not do anything (yet)");
};
/*jslint unparam: false */


/** \brief Get the column dependencies of this feature.
 *
 * Most of the features require other specific features to be defined
 * before themselves. This list of dependencies is used for this purpose:
 * to make sure that the list of features for a product is properly
 * ordered.
 *
 * This dependency list is used to sort the features when generating
 * the columns of data. The order may be different when generating
 * the cart footer so there is another set of dependencies for that
 * purpose.
 *
 * By default the list is composed exclusively of the "ecommerce::basic"
 * feature because all features (outside of the "ecommerce::basic" feature
 * itself) must at least depend on the basic feature.
 *
 * For example, the Tax feature will depend on the basic and the shipping
 * feature. That way the tax can appear after the shipping (because in some
 * countries, shipping is taxed and thus it makes more sense to make tax
 * appear afterward.)
 *
 * Please make sure to include the name of the plugin as a namespace
 * in the dependencies so we can make 100% to avoid any clashes.
 *
 * \note
 * This list of dependencies is only to order the list of features. It does
 * NOT mean that a feature will work only if all is dependencies are
 * satisfied. Quite the contrary. The tax feature does not require the
 * shipping feature at all.
 *
 * \todo
 * There is a limit to what one can do with this list. At this point, we
 * will try to make it all work this way. However, if you create a plugin
 * which should be before plugin X, Y, or Z, then those plugins would
 * need to depend on your plugin... and that's not going to happen with
 * this feature.
 *
 * @return {!string}  A comma separated list of feature names.
 */
snapwebsites.eCommerceProductFeatureBase.prototype.getColumnDependencies = function() // virtual
{
    return "ecommerce::basic";
};


/** \brief Get the footer dependencies of this feature.
 *
 * Most of the features require other specific features to be defined
 * before themselves. This list of dependencies is used for this purpose:
 * to make sure that the list of features for a product is properly
 * ordered.
 *
 * This dependency list is used to sort the features when generating
 * the footer lines. The order may be different when generating
 * the columns of the cart so there is another set of dependencies for
 * that purpose.
 *
 * By default this list is empty. Note however that the "ecommerce::basic"
 * must never be a dependency of any of the other features to make sure that
 * the basic features footer line appears last (this is enforced internally.)
 *
 * For example, the Tax feature will depend on the shipping feature so it
 * will be possible to mark shipping as taxable (although we may already
 * have added the shipping in the main rows of data so the tax would anyway
 * apply.)
 *
 * Please make sure to include the name of the plugin as a namespace
 * in the dependencies so we can make 100% to avoid any clashes.
 *
 * \note
 * This list of dependencies is only to order the list of features. It does
 * NOT mean that a feature will work only if all is dependencies are
 * satisfied. Quite the contrary. The tax feature does not require the
 * shipping feature at all.
 *
 * \todo
 * There is a limit to what one can do with this list. At this point, we
 * will try to make it all work this way. However, if you create a plugin
 * which should be before plugin X, Y, or Z, then those plugins would
 * need to depend on your plugin... and that's not going to happen with
 * this feature.
 *
 * @return {!string}  A comma separated list of feature names.
 */
snapwebsites.eCommerceProductFeatureBase.prototype.getFooterDependencies = function() // virtual
{
    return "ecommerce::basic";
};



/** \brief Snap eCommerceProduct constructor.
 *
 * For each of the product you add to the Cart, one of these
 * is created on the client system.
 *
 * The class is marked as final because the cart is in charge
 * to create such objects and thus you cannot create a derived
 * that would then be used by the cart.
 *
 * \code
 * final class eCommerceProduct extends eCommerceProductBase
 * {
 * public:
 *      function eCommerceProduct(product_type: eCommerceProductType);
 *
 *      virtual function getDescription() : String;
 *      virtual function getQuantity() : Number;
 *      virtual function setQuantity(quantity: Number) : Void;
 *      virtual function getPrice() : Number;
 *      virtual function getColumns(index: Number, features: Array of eCommerceFeaturesBase, in out columns: eCommerceColumns) : Void;
 *      virtual function getFeatures(in out features: eCommerceProductFeatureBase, use_column_features: Boolean) : Void;
 *      virtual function getProductType() : eCommerceProductType;
 *
 * private:
 *      function readyFeatures_() : Void;
 *      function compareColumn_(a: eCommerceProductFeatureBase, b: eCommerceProductFeatureBase) : Number;
 *      function compareFooter_(a: eCommerceProductFeatureBase, b: eCommerceProductFeatureBase) : Number;
 *
 *      var productType_: eCommerceProductType = null;
 *      var productColumnFeatures_: eCommerceProductFeatureBase = []; // sorted for columns
 *      var productFooterFeatures_: eCommerceProductFeatureBase = []; // sorted for footers
 *      var quantity_: number;
 * };
 * \endcode
 *
 * @param {snapwebsites.eCommerceProductType} product_type  A reference to the type to use for this product.
 *
 * @return {snapwebsites.eCommerceProduct} The newly created object.
 *
 * @constructor
 * @extends snapwebsites.eCommerceProductBase
 * @struct
 */
snapwebsites.eCommerceProduct = function(product_type)
{
    var quantity = product_type.getProperty("ecommerce::start_quantity");

    snapwebsites.eCommerceProduct.superClass_.constructor.call(this);

    this.productType_ = product_type;
    this.productColumnFeatures_ = [];
    this.productFooterFeatures_ = [];

    // force a default quantity; it cannot be zero because when that is
    // used the item is removed from the cart
    if(!quantity || snapwebsites.castToNumber(quantity, "somehow ecommerce::start_quantity is not a number") <= 0)
    {
        this.quantity_ = 1;
    }
    else
    {
        this.quantity_ = snapwebsites.castToNumber(quantity, "somehow ecommerce::start_quantity is not a number");
    }

    return this;
};


/** \brief Mark eCommerceProduct as inheriting from eCommerceProductBase.
 *
 * This class inherit from the eCommerceProductBase so it can be referenced
 * in lower level objects.
 */
snapwebsites.inherits(snapwebsites.eCommerceProduct, snapwebsites.eCommerceProductBase);


/** \brief A reference to the product type.
 *
 * This reference points to the product type which gives the
 * eCommerceProduct object access to the list of features used
 * by that product.
 *
 * \note
 * Initialized by the constructor.
 *
 * @type {snapwebsites.eCommerceProductType}
 * @private
 */
snapwebsites.eCommerceProduct.prototype.productType_ = null;


/** \brief An array of product features sorted for columns.
 *
 * This parameter holds the list of features attached to this
 * product. The minimum is the basic feature that all products have.
 *
 * @type {Array.<snapwebsites.eCommerceProductFeatureBase>}
 * @private
 */
snapwebsites.eCommerceProduct.prototype.productColumnFeatures_; // = []; initialized in constructor to avoid problems


/** \brief An array of product features sorted for footers.
 *
 * This parameter holds the list of features attached to this
 * product. The minimum is the basic feature that all products have.
 *
 * @type {Array.<snapwebsites.eCommerceProductFeatureBase>}
 * @private
 */
snapwebsites.eCommerceProduct.prototype.productFooterFeatures_; // = []; initialized in constructor to avoid problems


/** \brief The quantity entered by the customer.
 *
 * This parameter represents the numeric quantity the customer is
 * purchasing. Depending on the product and features, the quantity
 * may have some constrained (i.e. multiple of 100, not less than
 * 10, integer, allow 3 decimals, etc.)
 *
 * The default is taken from the product type which in most cases
 * is one (1).
 *
 * @type {number}
 * @private
 */
snapwebsites.eCommerceProduct.prototype.quantity_ = 0;


/** \brief Get the name of this widget.
 *
 * On creation the widget retrieves the attribute named "field_name"
 * which contains the name of the widget (used to communicate
 * with the server.) This function returns that name.
 *
 * @return {snapwebsites.eCommerceProductType}  A reference to the product type.
 *
 * @final
 */
snapwebsites.eCommerceProduct.prototype.getProductType = function()
{
    return this.productType_;
};


/** \brief Setup the features of this product.
 *
 * This function is called whenever the features of this products
 * are required. By default the features are not setup to avoid
 * wasting time (especially on a page with 100's of products in
 * a list.)
 *
 * @private
 */
snapwebsites.eCommerceProduct.prototype.readyFeatures_ = function()
{
    var cart,
        i,
        j,
        basic_feature,
        feature,
        feature_names,
        max_names,
        max_features,
        names,
        order;

    // if array is not empty, then the features were already readied
    // (no need to test whether Footer is empty, it is)
    if(this.productColumnFeatures_.length == 0)
    {
        cart = snapwebsites.eCommerceCartInstance;
        basic_feature = cart.getProductFeature("ecommerce::basic");
        this.productColumnFeatures_.push(basic_feature);

        // we want to sort the features here
        // we sort them by column dependencies and footer dependencies
        feature_names = this.productType_.getProperty("ecommerce::features");
        names = feature_names.split(",");
        max_names = names.length;
        for(i = 0; i < max_names; ++i)
        {
            // skip empty names (two commas in a row)
            // also skip the basic feature, we already added it!
            if(!names[i] || names[i] == "ecommerce::basic")
            {
                continue;
            }

            // if the feature does not exist, then the call will throw
            // so we do not have to do such a test here
            feature = cart.getProductFeature(names[i]);
            max_features = this.productColumnFeatures_.length;

            // first sort by column
            order = -1;
            for(j = 0; j < max_features; ++j)
            {
                order = this.compareColumn_(feature, this.productColumnFeatures_[j]);
                if(order < 0)
                {
                    this.productColumnFeatures_.splice(j, 0, feature);
                    break;
                }
            }
            if(order >= 0)
            {
                // it was not picked up yet, put at the end
                this.productColumnFeatures_.push(feature);
            }

            // second sort by footer
            order = -1;
            for(j = 0; j < max_features; ++j)
            {
                order = this.compareFooter_(feature, this.productFooterFeatures_[j]);
                if(order < 0)
                {
                    this.productFooterFeatures_.splice(j, 0, feature);
                    break;
                }
            }
            if(order >= 0)
            {
                // it was not picked up yet, put at the end
                this.productFooterFeatures_.push(feature);
            }
        }

        this.productFooterFeatures_.push(basic_feature);
    }
};


/** \brief Compare two features between each others.
 *
 * This function compares feature 'a' against 'b'. If 'b' depends
 * on 'a', then the function returns -1 meaning that 'a' must be handled
 * before 'b'. Otherwise the function returns 0.
 *
 * Features can be given a list of dependencies which is used to generate
 * a graph and order the features properly.
 *
 * @param {snapwebsites.eCommerceProductFeatureBase} a  The left hand side feature.
 * @param {snapwebsites.eCommerceProductFeatureBase} b  The right hand side feature.
 *
 * @private
 * @return {number} -1 if a has to be handled before b, 0 otherwise
 */
snapwebsites.eCommerceProduct.prototype.compareColumn_ = function(a, b)
{
    var an = "," + a.getFeatureName() + ",",
        bn = "," + b.getFeatureName() + ",",
        ad,
        bd,
        ap,
        bp;

    ad = "," + a.getColumnDependencies() + ",";
    bd = "," + b.getColumnDependencies() + ",";

    ap = bd.indexOf(an) == -1;
    bp = ad.indexOf(bn) == -1;

    if(!ap && !bp)
    {
        throw new Error("Two features depend on each other and thus cannot be sorted.");
    }

    return ap ? -1 : 0;
};


/** \brief Compare two features between each others.
 *
 * This function compares feature 'a' against 'b'. If 'b' depends
 * on 'a', then the function returns -1 meaning that 'a' must be handled
 * before 'b'. Otherwise the function returns 0.
 *
 * Features can be given a list of dependencies which is used to generate
 * a graph and order the features properly.
 *
 * This function checks the Footer dependencies.
 *
 * @param {snapwebsites.eCommerceProductFeatureBase} a  The left hand side feature.
 * @param {snapwebsites.eCommerceProductFeatureBase} b  The right hand side feature.
 *
 * @private
 * @return {number} -1 if a has to be handled before b, 0 otherwise
 */
snapwebsites.eCommerceProduct.prototype.compareFooter_ = function(a, b)
{
    var an = "," + a.getFeatureName() + ",",
        bn = "," + b.getFeatureName() + ",",
        ad,
        bd,
        ap,
        bp;

    ad = "," + a.getFooterDependencies() + ",";
    bd = "," + b.getFooterDependencies() + ",";

    ap = bd.indexOf(an) == -1;
    bp = ad.indexOf(bn) == -1;

    if(!ap && !bp)
    {
        throw new Error("Two features depend on each other and thus cannot be sorted.");
    }

    return ap ? -1 : 0;
};


/** \brief Merge two arrays of column features.
 *
 * This function is used to merge the array of features passed in and
 * the array of features of this product.
 *
 * This is used to generate the array of features necessary to generate
 * the cart header. We may later want to find a better way to handle
 * this special case. One way would be to force all features to always
 * add all their column in all the products, but right now we do NOT
 * force a product to make use of all the features (i.e. one product
 * may have taxes, but no shipping, another may have neither, one
 * may have both... thus you may get 1 or 2 columns differences.)
 *
 * @param {Array.<snapwebsites.eCommerceProductFeatureBase>} features  A reference to an array of features.
 * @param {boolean} use_column_features  If true, use the column features, if false, use the footer features.
 *
 * @final
 */
snapwebsites.eCommerceProduct.prototype.getFeatures = function(features, use_column_features)
{
    var max_features,   // WARNING: productColumnFeatures may not be ready here so we cannot get the length now
        max,
        i,
        j,
        pos = 0,
        done,
        name,
        product_features = use_column_features ? this.productColumnFeatures_ : this.productFooterFeatures_,
        f;

    // First we have to make sure that the features are ready
    this.readyFeatures_();

    // The order may not be the same so this loop is really complicated...
    max_features = product_features.length;
    for(i = 0; i < max_features; ++i)
    {
        f = product_features[i];
        name = f.getFeatureName();

        // is it the next feature in the existing list?
        done = false;
        if(pos < features.length)
        {
            if(features[pos].getFeatureName() == name)
            {
                // it already exists, we are good
                ++pos;
                done = true;
            }
        }
        if(!done)
        {
            // make sure we do not have it somewhere else
            // (possibly before so we start with 0 and not 'pos')
            max = features.length;
            for(j = 0; j < max; ++j)
            {
                if(features[j].getFeatureName() == name)
                {
                    // make sure 'pos' does not go backward
                    if(pos < j + 1)
                    {
                        pos = j + 1;
                    }
                    done = true;
                    break;
                }
            }

            if(!done)
            {
                // put at the end
                features.push(f);
                pos = features.length;
            }
        }
    }
};


/** \brief Generates the columns for the cart.
 *
 * This function allocates a columns object and then adds the columns
 * as required by calling the necessary function on all the features of
 * this product.
 *
 * Note that it always calls the Basic feature first. Other features
 * can be ordered as required.
 *
 * \note
 * At this point we do not really need this function to be a product
 * function, but I guess it makes more sense.
 *
 * @param {number} index  The line counter.
 * @param {Array.<snapwebsites.eCommerceProductFeatureBase>} features  A reference to an array of features.
 * @param {!snapwebsites.eCommerceColumns} columns  A reference to the product type.
 *
 * @final
 */
snapwebsites.eCommerceProduct.prototype.getColumns = function(index, features, columns)
{
    var max_features = features.length,
        i;

    for(i = 0; i < max_features; ++i)
    {
        features[i].setupColumns(this, columns, index);
    }
};


/** \brief Return the product description.
 *
 * This function returns the product description.
 *
 * @return {string}  The brief description to use in the cart for this product.
 * @final
 */
snapwebsites.eCommerceProduct.prototype.getDescription = function()
{
    return snapwebsites.castToString(this.productType_.getProperty("ecommerce::description"), "product ecommerce::description is not a valid string");
};


/** \brief Return the product quantity.
 *
 * This function sets the new product quantity.
 *
 * The function accepts a quantity of zero because in some very rare
 * but non the less existing cases... However, negative quantities
 * are refused.
 *
 * @param {number} quantity  The new quantity of this product in the cart.
 *
 * @final
 */
snapwebsites.eCommerceProduct.prototype.setQuantity = function(quantity) // virtual
{
    // TBD: should this test be done in the base class and here we would
    //      call the base class?
    if(quantity < 0)
    {
        throw new Error("snapwebsites.eCommerceProduct.setQuantity() does not accept negative quantities (" + quantity + ").");
    }
    this.quantity_ = quantity;
};


/** \brief Return the product quantity.
 *
 * This function returns the product quantity.
 *
 * @return {number}  The quantity of this product in the cart.
 * @final
 */
snapwebsites.eCommerceProduct.prototype.getQuantity = function()
{
    return this.quantity_;
};


/** \brief Return the product price.
 *
 * This function returns the product price.
 *
 * @return {number}  The sale price of this product.
 * @final
 */
snapwebsites.eCommerceProduct.prototype.getPrice = function()
{
    return snapwebsites.castToNumber(this.productType_.getProperty("ecommerce::price"), "product ecommerce::price is not a valid number");
};



/** \brief Snap eCommerceCart constructor.
 *
 * The eCommerceCart includes everything required to handle the following:
 *
 * - Add a product to the cart
 * - Edit the cart / wishlist
 *   - Global functions
 *     - Open (show)
 *     - Close (hide)
 *     - Clear cart
 *     - Checkout (button to "move" to checkout)
 *   - Actual editing
 *     - Delete a product
 *     - Change product quantity (if allowed by product type)
 *     - Move product from cart to wishlist (and vice versa from wishlist)
 *     - Links to products for details
 *   - Sharing of wishlist (so others buy the items on it for you!)
 * - Send changes to the server
 *   - Handle server response
 *
 * This class automatically connects against all the links marked with
 * the ".buy-now-button" class (i.e. it has to be an anchor.) You can
 * implemented your own way to add products to the cart using this
 * function:
 *
 * \code
 *  guid = "...url of product...";
 *  eCommerceCartInstance.addProduct(guid);
 *    --- or to add 3 at once ---
 *  eCommerceCartInstance.addProduct(guid, 3);
 * \endcode
 *
 * The product GUID must be a product that was loaded when the current
 * page was loaded. You cannot yet add a product that was not yet loaded.
 * (Certainly a capability we want to offer at some point, using AJAX to
 * get the product details...)
 *
 * Note that this class is marked as final because it is used as a
 * singleton. To extend the cart you have to create product features.
 * Those features can be used in various different ways to extend
 * pretty much all the parts of the cart.
 *
 * \code
 * final class eCommerceCart extends ServerAccessTimerCallbacks
 * {
 * public:
 *      eCommerceCart();
 *
 *      function registerProductFeature(product_feature: eCommerceProductFeatureBase) : Void;
 *      function hasProductFeature(feature_name: string) : boolean;
 *      function getProductFeature(feature_name: string) : eCommerceProductFeatureBase;
 *
 *      function registerProductType(data: Object) : eCommerceCart;
 *      function addProduct(guid: String, opt_quantity: Number) : Void;
 *
 *      function checkModified();
 *
 *      function showGlimpse() : Void;
 *      function hideGlimpse() : Void;
 *      function showCart() : Void;
 *      function hideCart() : Void;
 *
 *      function getTotalQuantity() : Number;
 *      function getTotalCosts() : Number;
 *
 *      virtual function serverAccessTimerReady(request_name: string, server_access: ServerAccess) : Void;
 *
 * private:
 *      function generateCartHtml_(e: jQuery) : String;
 *      function generateCartHeader_(e: jQuery, in out columns: eCommerceColumns) : Void;
 *      function generateCartFooter_(e: jQuery, in out columns: eCommerceColumns) : Void;
 *      function generateProductTable_(e: jQuery, in out columns: eCommerceColumns, columns_features: Array of eCommerceProductFeatureBase, footer_features: Array of eCommerceProductFeatureBase) : Void;
 *      function generateProductTableHeader_(e: jQuery, in out columns: eCommerceColumns) : Void;
 *      function generateProductTableFooter_(e: jQuery, in out columns: eCommerceColumns, footer_features: Array of eCommerceProductFeatureBase) : Void;
 *      function generateProductTableRow_(e: jQuery, row_index: Number, in out columns: eCommerceColumns) : Void;
 *      function getColumnHeaders_(features: Array of eCommerceProductFeatureBase, in out columns: eCommerceColumns) : Void;
 *      function getColumnFooters_(features: Array of eCommerceProductFeatureBase, in out columns: eCommerceColumns, index: Number) : Void;
 *
 *      static var createPopup_: Object;
 *      var productFeatures_: eCommerceProductFeatureBase;
 *      var productTypes_: Object; // indexed by product type GUID
 *      var cartPaymentDefined_: Boolean;
 *      var cartHtml_: jQuery;
 *      var products_: Array of eCommerceProductBase;
 * };
 * \endcode
 *
 * \todo
 * Add functionality to add products by URL only, i.e. without having to
 * load the product at the time the page is hit. This means we need to
 * use AJAX to retrieve the product details at the time the product gets
 * added to the cart. We could then have a "wait" image in the cart to
 * show that it is currently working.
 *
 * @return {snapwebsites.eCommerceCart}  The newly created object.
 *
 * @constructor
 * @extends {snapwebsites.ServerAccessTimerCallbacks}
 * @struct
 */
snapwebsites.eCommerceCart = function()
{
    var that = this;

//#ifdef DEBUG
    if(jQuery("body").hasClass("snap-cart-initialized"))
    {
        throw new Error("Only one cart singleton can be created.");
    }
    jQuery("body").addClass("snap-cart-initialized");
//#endif
    snapwebsites.eCommerceCart.superClass_.constructor.call(this);

    this.productFeatures_ = {};
    this.productTypes_ = {};  // types do not have a name, there is a product name though
    this.products_ = [];

    // now connect to Buy Now buttons and maybe more too...
    jQuery("a.buy-now-button").click(function(e)
        {
            var guid = jQuery(this).attr("product-guid");

            e.preventDefault();
            e.stopPropagation();

            that.addProduct(snapwebsites.castToString(guid, "somehow ecommerce::start_quantity is not a number"));
        });

    return this;
};


/** \brief Mark the eCommerceCart as inheriting from the ServerAccessTimerCallbacks.
 *
 * This class inherits from the ServerAccessTimerCallbacks, which it uses
 * to send the server changes made by the client to the cart.
 *
 * In the cart, that feature is pretty much always asynchroneous.
 *
 * Remember that the ServerAccessTimerCallbacks is itself derived from the
 * ServerAccessCallbacks so you get the same set of callbacks plus whatever
 * the timer adds to the "interface".
 */
snapwebsites.inherits(snapwebsites.eCommerceCart, snapwebsites.ServerAccessTimerCallbacks);


/** \brief The popup information for the Snap! Website cart functionality.
 *
 * This static object represents the popup object used by the Cart to
 * present to the user the currently selected products ready to be
 * purchased by going to the checkout area.
 *
 * @type {Object}
 * @private
 */
snapwebsites.eCommerceCart.createPopup_ = // static
{

// WARNING: we have exactly ONE instance of this variable
//          (i.e. if we were to create more than one Main object
//          we would still have ONE instance.)

    id: "ecommerce-cart",
    title: "Cart",
    darken: 150,
    width: 750,
    height: 350
    //beforeHide: -- defined as required
};


/** \brief The list of product features understood by the e-Commerce cart.
 *
 * This map is used to save the product features when you call the
 * registerProductFeature() function.
 *
 * \note
 * The registered product features cannot be unregistered.
 *
 * @type {Object}
 * @private
 */
snapwebsites.eCommerceCart.prototype.productFeatures_; // = {}; -- initialized in constructor to avoid problems


/** \brief The list of product types one can add to the e-Commerce cart.
 *
 * This map is used to save the various product definitions. Product
 * definitions are used to allow the user to handle the products:
 *
 * \li Add products to the cart.
 * \li Display the products in the cart.
 * \li Compute the total costs.
 * \li Define various types of quantities.
 * \li Computer the grand total.
 * \li etc.
 *
 * The add of product types is done using the registerProductType()
 * function.
 *
 * The object saves each product type object using the GUID as the
 * index. The GUI is a mandatory parameter of the product type which
 * in most cases is the URL of the product.
 *
 * \note
 * The registered product types cannot be unregistered.
 *
 * @type {Object}
 * @private
 */
snapwebsites.eCommerceCart.prototype.productTypes_; // = {}; -- initialized in constructor to avoid problems


/** \brief Whether the cart payment facilities were generated.
 *
 * By default, the HTML for the checkout pages is not generated.
 * This flag is used to know whether it got generated since then.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.eCommerceCart.prototype.cartPaymentDefined_ = false;


/** \brief A variable member referencing the cart HTML.
 *
 * This jQuery object is the one returned by the open() function
 * of the Popup class. In other words, it is not directly the
 * part where we can add the cart. You must make sure to add
 * code as children of the ".popup-body" div. Not doing so may
 * interfere with the popup code and other parts of the cart.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.eCommerceCart.prototype.cartHtml_ = null;


/** \brief The list of products that have been added to the cart.
 *
 * This array is the list of products that have been added to the cart.
 *
 * @type {Array.<snapwebsites.eCommerceProductBase>}
 * @private
 */
snapwebsites.eCommerceCart.prototype.products_; // = []; -- initialized in constructor to avoid problems


/** \brief Keep the time when we last sent a request to the server.
 *
 * In order to avoid swamping the server with cart requests, we keep a
 * time. The timing can be changed by the administrator. By default we
 * use 2 seconds which in general is enough because those requests are
 * relatively small.
 *
 * Do not forget that when a user wants to leave a page, the cart has
 * to have been sent. So too large an interval could cause a real
 * problem.
 *
 * @type {snapwebsites.ServerAccessTimer}
 * @private
 */
snapwebsites.eCommerceCart.prototype.serverAccessTimer_ = null;


/** \brief Avoid sending cart to the server when initializing.
 *
 * When reloading a page we get to reload the cart with the addProduct()
 * function. This has the side effect to send the information back to
 * the server with a POST. When this flag is set to true, the sendCart_()
 * function does nothing.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.eCommerceCart.prototype.initializing_ = false;


/** \brief Interval between two POSTs to the server.
 *
 * The minimum amount of time between two requests to be sent to the
 * sever. Note that whatever this amount, if a request was not yet
 * completed, then nothing happens.
 *
 * @type {number}
 * @private
 */
snapwebsites.eCommerceCart.prototype.requestsInterval_ = 2000;


/** \brief Set whether we are initializing the cart or not.
 *
 * On load, the cart gets initialized using the same functions as when
 * being used. This means adding items to the cart generates POST events
 * to the server. Using this flag, we can turn off the POST messages.
 *
 * @param {boolean} status  Whether you are initializing (true) or not (false).
 *
 * @return {snapwebsites.eCommerceCart}  The cart so we can chain calls.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.setInitializing = function(status)
{
    this.initializing_ = status;
    return this;
};


/** \brief Register a product feature.
 *
 * This function is used to register an e-Commerce product feature in the
 * cart.
 *
 * This allows for any number of extensions and thus any number of
 * cool advanced features that do not all need to be defined in the
 * core of the cart implementation.
 *
 * @param {snapwebsites.eCommerceProductFeatureBase} product_feature  The product feature to register.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.registerProductFeature = function(product_feature)
{
    var name = product_feature.getFeatureName();
    this.productFeatures_[name] = product_feature;
};


/** \brief Check whether a feature exists.
 *
 * This function checks the list of product features to see whether
 * \p feature_name exists.
 *
 * @param {!string} feature_name  The name of the feature to check.
 *
 * @return {!boolean}  true if the feature is defined.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.hasProductFeature = function(feature_name)
{
    return this.productFeatures_[feature_name] instanceof snapwebsites.eCommerceProductFeatureBase;
};


/** \brief This function is used to get a product feature.
 *
 * Product features get registered with the registerProductFeature() function.
 * Later you may retrieve them using this function.
 *
 * @throws {Error} If the named \p feature_name was not yet registered,
 *                 then this function throws.
 *
 * @param {string} feature_name  The name of the product feature to retrieve.
 *
 * @return {snapwebsites.eCommerceProductFeatureBase}  The product feature object.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.getProductFeature = function(feature_name)
{
    if(this.productFeatures_[feature_name] instanceof snapwebsites.eCommerceProductFeatureBase)
    {
        return this.productFeatures_[feature_name];
    }
    throw new Error("getProductFeature(\"" + feature_name + "\") called when that feature is not yet defined.");
};


/** \brief Register a product type.
 *
 * This function is used to register an e-Commerce product type in the
 * cart. Product types are used to add products to the cart.
 *
 * This allows for any number of products to be added to the cart,
 * however, generally only the following types of product type
 * definitions are loaded on a page:
 *
 * 1. The product represented by the page, or in case of a list, all the
 *    different products shown in that list (later we will have auto-loading
 *    lists that will dynamically grow the list of possible product types.)
 *    We may also have products defined in boxes.
 *
 * 2. The products that are promoted.
 *
 * 3. The products already defined in the cart.
 *
 * @throws {Error}  The data MUST have a GUID defined.
 *
 * @param {Object} data  The product type data.
 *
 * @return {snapwebsites.eCommerceCart}  Return a reference to the e-Commerce
 *                                       cart so you can chain the
 *                                       registration calls.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.registerProductType = function(data)
{
    // in this case we create the object
    //
    // the 'data' parameter defines the product type data, however,
    // for most that 'data' is handled by product features
    var product_type = new snapwebsites.eCommerceProductType(data),
        guid = product_type.getProperty("ecommerce::guid");

    this.productTypes_[guid] = product_type;

    return this;
};


/** \brief Send a copy of the cart to the server.
 *
 * This function creates a POST and sends it to the server with the current
 * content of the cart.
 *
 * \todo
 * Implement an incremental process. Although this would be useful only for
 * really large carts, but we do not really have to support such right now.
 *
 * @private
 */
snapwebsites.eCommerceCart.prototype.sendCart_ = function()
{
    if(this.initializing_)
    {
        return;
    }
    if(!this.serverAccessTimer_)
    {
        this.serverAccessTimer_ = new snapwebsites.ServerAccessTimer("cart", this, this.requestsInterval_);
    }
    this.serverAccessTimer_.send();
};


/** \brief This function adds a product to the cart.
 *
 * In most cases this is called when people click on a "buy-now-button"
 * (a link marked with that class).
 *
 * If the product is not defined, then we generate an error (it should
 * always be available when this function gets called.)
 *
 * The cart is automatically updated if required.
 *
 * @param {string} guid  The GUID of the product type to be added.
 * @param {number=} opt_quantity  The quantity if add a new object.
 */
snapwebsites.eCommerceCart.prototype.addProduct = function(guid, opt_quantity)
{
    var max = this.products_.length,
        product_type = this.productTypes_[guid],
        product,
        other_product_type,
        found = false,
        maximum_count,
        add,
        new_quantity,
        i;

    if(!(product_type instanceof snapwebsites.eCommerceProductType))
    {
        // that product type does not exist?!
        snapwebsites.OutputInstance.displayOneMessage("Missing Product",
                    "Somehow we could not find product at " + guid);
        return;
    }

    // first we want to see whether that object was already
    // added to the cart; if so the only reason to add another
    // is in case you have a different attribute to the product
    // which right now we do not yet support (a serial number
    // is also an attribute)

    for(i = 0; i < max; ++i)
    {
        other_product_type = this.products_[i].getProductType();
        if(other_product_type.getProperty("ecommerce::guid") == guid)
        {
            // hmmm... the user already has this product in his cart,
            // ask whether we should add one item or just forget the
            // add altogether
            found = true;
            add = other_product_type.getProperty("ecommerce::add_quantity");
            if(!add || snapwebsites.castToNumber(add, "somehow ecommerce::add_quantity is not a number") < 0)
            {
                add = 1;
            }
            new_quantity = this.products_[i].getQuantity() + snapwebsites.castToNumber(add, "somehow ecommerce::add_quantity is not a number");
            maximum_count = other_product_type.getProperty("ecommerce::maximum_count");
            if(!maximum_count || snapwebsites.castToNumber(maximum_count, "somehow ecommerce::maximum_count is not a number") < 0)
            {
                maximum_count = 100;
            }
            if(new_quantity > snapwebsites.castToNumber(maximum_count, "somehow ecommerce::maximum_count is not a number"))
            {
                // TODO: translation -- should come from settings too
                snapwebsites.OutputInstance.displayOneMessage("Maximum Reached",
                            "You already reached the maximum number of items of this type in your cart. "
                          + "If you need more, please checkout with your current cart. Then pass another order.",
                            "warning");
                return;
            }
            this.products_[i].setQuantity(new_quantity);
            break;
        }
    }

    if(!found)
    {
        // TODO: define this maximum count in the cart settings; this is
        //       not specific to a product type so we cannot just retrieve
        //       a property from a product type...
        maximum_count = 100;
        if(this.products_.length >= maximum_count)
        {
            // TODO: translation -- should come from settings too
            snapwebsites.OutputInstance.displayOneMessage("Maximum Reached",
                        "You already reached the maxmimum number of items you can add to your cart. "
                      + "Please checkout now and order further goods in a second order.");
            return;
        }
        product = new snapwebsites.eCommerceProduct(product_type);
        this.products_.push(product);
        if(opt_quantity)
        {
            product.setQuantity(opt_quantity);
        }
    }

    this.sendCart_();
    if(this.cartHtml_)
    {
        // re-generate (this is because we do not yet have all the
        // necessary callbacks)
        this.generateCartHtml_();
    }
    this.showCart();
};


/** \brief This function removes a product from the cart.
 *
 * In most cases this is called when people click on a "cart-delete-product"
 * (a link marked with that class). It removes all the products with the
 * specified \p guid from the cart.
 *
 * The cart is automatically updated if required.
 *
 * @param {string} guid  The GUID of the product type to be added.
 */
snapwebsites.eCommerceCart.prototype.removeProduct = function(guid)
{
    var i = this.products_.length,
        product_type = this.productTypes_[guid],
        other_product_type,
        modified = false;

    if(!(product_type instanceof snapwebsites.eCommerceProductType))
    {
        // that product type does not exist?!
        // ignore the error (no throw) so you can have a hard coded
        // URI and it just gets ignored if the product is not loaded
        return;
    }

    // search for the product in the cart and delete all instances
    //
    // TODO: we will need to fix that because right now it ignores the
    //       attributes (which is a very important aspect of the cart)
    //       we should be able to use the row id which would be 'i' in
    //       the following loop
    while(i > 0)
    {
        --i;
        other_product_type = this.products_[i].getProductType();
        if(other_product_type.getProperty("ecommerce::guid") == guid)
        {
            // found a copy, get rid of it
            this.products_.splice(i, 1);
            modified = true;
        }
    }

    if(modified)
    {
        this.sendCart_();
        this.generateCartHtml_();
    }
};


/** \brief This function removes all the products from the cart.
 *
 * This function loops through all the products currently in the cart
 * and removes them all.
 *
 * The cart is automatically updated if required.
 */
snapwebsites.eCommerceCart.prototype.clearCart = function()
{
    var i = this.products_.length,
        modified = i > 0;

    // got through the list of all the products in the cart and delete them
    while(i > 0)
    {
        --i;
        this.products_.splice(i, 1);
    }

    if(modified)
    {
        this.sendCart_();
        this.generateCartHtml_();
    }
};


/** \brief Check whether a field in the cart editor form was modified.
 *
 * When a quantity or some other parameter was modified in the cart,
 * this function is called to check whether it looks different. If so
 * then the change is sent to the server for archiving.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.checkModified = function()
{
    this.showCart();
};


/** \brief Show a "glimpse" of the cart.
 *
 * This function shows the cart icon in your theme. The theme has
 * a certain amount of control on how the cart appears as a glimpse.
 *
 * If the cart is empty, the empty cart icon is shown.
 *
 * If the cart is not empty, the standard cart icon is shown.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.showGlimpse = function()
{
};


/** \brief Hide the "glimpse" of the cart.
 *
 * This function hides the cart icon from the theme. This means the
 * cart is not directly accessible by the end user which is certainly
 * a good idea on certain pages (i.e. a form), not so good on others.
 *
 * For example, it is generally a good idea to hide the cart and
 * glimpse when asking the user for his credit card information
 * (card number, address, phone...)
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.hideGlimpse = function()
{
};


/** \brief Show the cart.
 *
 * This function shows the cart in a popup. This is 100% done on the
 * client side (no access to the server.) This is because the cart
 * is 100% built using code.
 *
 * The cart makes use of a fully dynamic editor form.
 *
 * In most cases, clicking on the Glimpse will trigger a call to this
 * function. Other methods may be used to open the cart.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.showCart = function()
{
    var e;

    if(!this.cartHtml_)
    {
        this.cartHtml_ = snapwebsites.PopupInstance.open(snapwebsites.eCommerceCart.createPopup_);
        e = this.cartHtml_.find(".popup-body");
        e.append(
                "<div class='cart-summary'></div>"
              + "<div class='cart-payment'></div>"
            );
        this.generateCartHtml_();
        // the cart-payment HTML is created later and only if the user
        // goes to the checkout area
    }
    // probably need an update here?
    // although even when hidden we probably want to keep this up to date
    // through the checkModified() function

    snapwebsites.PopupInstance.show(snapwebsites.eCommerceCart.createPopup_);
};


/** \brief Hide the cart.
 *
 * This function hides the cart, really it just closes the popup
 * opened by the showCart() function. The close button of the popup
 * will do the same thing.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.hideCart = function()
{
    snapwebsites.PopupInstance.hide(snapwebsites.eCommerceCart.createPopup_);
};


/** \brief Retrieve the current total quantity.
 *
 * This function goes through the list of products and computes the
 * total quantity of products being purchased.
 *
 * The number of products represents the total number of different
 * items. So if some products have a quantity larger than 1, the
 * total is smaller than the total number of items which this
 * function calculates.
 *
 * \note
 * It probably does not make sense to show a total quantity if
 * the various items you can add to your cart use varied measurement
 * systems (i.e. 3 grammes of gold and 3 kg of apples..., what would
 * 6 mean here?)
 *
 * \todo
 * We may have to properly count the quantity in the main cart
 * list or in the wishlist.
 *
 * @return {number}  The total number of items.
 */
snapwebsites.eCommerceCart.prototype.getTotalQuantity = function()
{
    var i,
        max = this.products_.length,
        total = 0;

    for(i = 0; i < max; ++i)
    {
        total += this.products_[i].getQuantity();
    }

    return total;
};


/** \brief Retrieve the current total costs.
 *
 * This function goes through the list of products and computes the
 * total costs of products being purchased.
 *
 * If the tax and shipping methods used include these as products,
 * then those numbers will be included in this total. Otherwise,
 * it should be the net costs.
 *
 * \todo
 * Define the number of digits to keep behind the decimal point.
 * Right now that is hard coded as 2 in several places.
 *
 * @return {number}  The total number of items.
 */
snapwebsites.eCommerceCart.prototype.getTotalCosts = function()
{
    var i,
        max = this.products_.length,
        total = 0;

    // compute the total in cents
    for(i = 0; i < max; ++i)
    {
        // we MUST round on a per computation because that is
        // what we present the end user on each line of the cart!
        //
        // TODO: add the per line taxes/shipping/etc.
        total += Math.round(this.products_[i].getPrice() * this.products_[i].getQuantity() * 100);
    }

    return total / 100;
};


/** \brief Generate the cart HTML code.
 *
 * This function generates the HTML code using jQuery() to append the
 * code as required.
 *
 * The function first calls the generate_cart_header() function which
 * can be overridden since it is virtual.
 *
 * Next it calls the generate_product_table() which is expected to create
 * the table with the items currently in the cart. If no item are defined,
 * it still creates a table, only it will be marked as hidden and instead
 * we show another div tag with the message "your cart is empty".
 *
 * We do not allow the generate_product_table() function to be overridden.
 * Instead we expect the product types to generate the data accordingly
 * so the dynamism is not available in the table itself. However, as you
 * can see in the function definition, the table header and footer functions
 * can be overridden.
 *
 * Finally the function calls the generate_cart_footer() function which
 * can be overridden too since it is virtual.
 *
 * @private
 * @final
 */
snapwebsites.eCommerceCart.prototype.generateCartHtml_ = function()
{
    // first we generate an array of product columns that we want to
    // include in the resulting table displayed in the cart.
    var that = this,
        i,
        max = this.products_.length,
        columns = new snapwebsites.eCommerceColumns(),
        column_features = [],
        footer_features = [],
        e = this.cartHtml_.find(".popup-body .cart-summary");

    // make sure we start with an empty cart, everything below
    // uses append() to add components to the cart
    e.empty();

    // First take the complete list of features necessary
    // to build the headers; then generate the headers
    if(max == 0)
    {
        // no products... get the default feature
        column_features.push(this.getProductFeature("ecommerce::basic"));
    }
    else
    {
        for(i = 0; i < max; ++i)
        {
            this.products_[i].getFeatures(column_features, true);
        }
    }
    this.getColumnHeaders_(column_features, columns);

    // now that we have a complete list of all the features
    columns.generateColumnsMap();

    for(i = 0; i < max; ++i)
    {
        this.products_[i].getColumns(i, column_features, columns);
    }

    this.getColumnFooters_(column_features, columns, i);

    // the footer features are not sorted in the same order so we
    // have to get these too, like we did with the headers
    for(i = 0; i < max; ++i)
    {
        this.products_[i].getFeatures(footer_features, false);
    }

    this.generateCartHeader_(e, columns);
    this.generateProductTable_(e, columns, column_features, footer_features);
    this.generateCartFooter_(e, columns);

    // now connect to various buttons
    e.find(".clear-cart").click(function(e)
        {
            e.preventDefault();
            e.stopPropagation();

            that.clearCart();
        }).toggleClass("disabled", max == 0);
    e.find(".continue-shopping").click(function(e)
        {
            e.preventDefault();
            e.stopPropagation();

            that.hideCart();
        });
    e.find(".checkout").click(function(e)
        {
            e.preventDefault();
            e.stopPropagation();

            // is it disabled?
            if(max == 0)
            {
                return;
            }

            // TODO: avoid searching for the cart payment/summary multiple times
            if(!that.cartPaymentDefined_)
            {
                that.cartPaymentDefined_ = true;
                snapwebsites.ePaymentInstance.appendMainFacilities(that.cartHtml_.find(".popup-body .cart-payment"), 750, true);
                snapwebsites.ePaymentInstance.setCartModifiedCallback(function()
                    {
                        that.clearCart();
                        that.generateCartHtml_();
                        that.cartHtml_.find(".popup-body .cart-payment").hide();
                        that.cartHtml_.find(".popup-body .cart-summary").show();
                    });
            }
            that.cartHtml_.find(".popup-body .cart-summary").hide();
            that.cartHtml_.find(".popup-body .cart-payment").show();
        }).toggleClass("disabled", max == 0);
    e.find(".cart-delete-product").click(function(e)
        {
            var guid = snapwebsites.castToString(jQuery(this).attr("guid"), "product ecommerce::description is not a valid string");

            e.preventDefault();
            e.stopPropagation();

            that.removeProduct(guid);
        });
};


/** \brief Generates the column headers for the cart.
 *
 * This function adds the column headers to the columns.
 *
 * @param {Array.<snapwebsites.eCommerceProductFeatureBase>} features  A reference to an array of features.
 * @param {!snapwebsites.eCommerceColumns} columns  A reference to the columns.
 *
 * @private
 * @final
 */
snapwebsites.eCommerceCart.prototype.getColumnHeaders_ = function(features, columns)
{
    var max_features = features.length,
        i;

    for(i = 0; i < max_features; ++i)
    {
        features[i].setupColumnHeaders(columns);
    }
};


/** \brief Generates the column footers for the cart.
 *
 * This function adds the column footers to the columns object.
 *
 * This includes lines such as the shipping, taxes, grand total.
 *
 * @param {Array.<snapwebsites.eCommerceProductFeatureBase>} features  A reference to an array of features.
 * @param {!snapwebsites.eCommerceColumns} columns  A reference to the columns.
 * @param {number} index  The row index for this product entry.
 *
 * @private
 * @final
 */
snapwebsites.eCommerceCart.prototype.getColumnFooters_ = function(features, columns, index)
{
    var max_features = features.length,
        i;

    for(i = 0; i < max_features; ++i)
    {
        features[i].setupColumnFooters(columns, index);
    }
};


/** \brief Generate a cart header.
 *
 * This function creates the header of the cart. This is generally
 * information about the store and such. It should remains really
 * brief (one line or two.)
 *
 * \todo
 * Implement a way for the store administrator to enter what should
 * appear in the header and make the count a parameter (i.e. using
 * the filter something like [ecommerce::total_quantity]) We want
 * to allow for filtering with some JS from the filter plugin to do
 * the work too!
 *
 * @param {!jQuery} e  The element where the header can directly be added.
 * @param {!snapwebsites.eCommerceColumns} columns  The price of the product.
 *
 * @private
 * @final
 */
snapwebsites.eCommerceCart.prototype.generateCartHeader_ = function(e, columns)
{
    var count,
        total;

    if(this.products_.length != 0)
    {
        total = this.getTotalQuantity();
        if(total == 1)
        {
            count = "One item";
        }
        else
        {
            count = total + " items";
        }
    }
    else
    {
        count = "No items";
    }
    e.append("<div class='cart-header'>" + count + "</div>");
};


/** \brief Generate a cart footer.
 *
 * The function creates the footer of the cart. This is generally
 * detailed information about the store letigimacy (i.e. certificates)
 * and eventually some reference to other products one can purchase.
 *
 * \todo
 * Make use of a "Thank you" note from the store administrator. Like
 * in the header, we want to allow for filtering (we want some JS from
 * the filter plugin to do the work too!)
 *
 * @param {!jQuery} e  The element where the footer can directly be added.
 * @param {!snapwebsites.eCommerceColumns} columns  The price of the product.
 *
 * @private
 * @final
 */
snapwebsites.eCommerceCart.prototype.generateCartFooter_ = function(e, columns)
{
    e.append(
            "<div class='cart-footer'>Thank you.</div>"
          + "<div class='cart-buttons'>"
            + "<a class='clear-cart' href='/clear-cart'>Clear Cart</a>"
            // checkout and continue-shopping are inverted by the CSS
            + "<a class='checkout' href='/checkout'>Check Out</a>"
            + "<a class='continue-shopping' href='/continue-shopping'>Continue Shopping</a>"
            + "<div style='clear: both;'></div>"
          + "</div>"
        );
};


/** \brief Generate the product table of the cart.
 *
 * This function generates a list of products in a table. It makes use
 * of the list of products available in the cart. In case all the products
 * get removed, we also offer a fallback when no products are defined in
 * the cart.
 *
 * @param {!jQuery} e  The element where the table can directly be added.
 * @param {!snapwebsites.eCommerceColumns} columns  The price of the product.
 * @param {Array.<snapwebsites.eCommerceProductFeatureBase>} column_features  A reference to an array of features.
 * @param {Array.<snapwebsites.eCommerceProductFeatureBase>} footer_features  A reference to an array of features.
 *
 * @private
 * @final
 */
snapwebsites.eCommerceCart.prototype.generateProductTable_ = function(e, columns, column_features, footer_features)
{
    var s,      // scrollarea
        t,      // table
        h,      // header
        b,      // body
        f,      // footer
        i,
        max = this.products_.length;

    // div around table for vertical sliderbar...
    e.append("<div class='cart-table-scrollarea'>");
    s = e.children("div.cart-table-scrollarea");

    // table
    s.append("<table class='cart-product-table'>");
    t = s.children("table");

    // header
    t.append("<thead>");
    h = t.children("thead");
    this.generateProductTableHeader_(h, columns);

    // footer (must appear before <tbody> to be HTML 4.1 compatible, fix in HTML 5)
    t.append("<tfoot>");
    f = t.children("tfoot");
    this.generateProductTableFooter_(f, columns, footer_features);

    // body
    t.append("<tbody>");
    b = t.children("tbody");

    if(max == 0)
    {
        this.generateProductTableEmpty_(b, columns);
    }
    else
    {
        for(i = 0; i < max; ++i)
        {
            this.generateProductTableRow_(b, i, columns);
        }
    }

    // TODO: optionally add one more line with totals of the cart entries;
    //       right now we do not offer that option at all
};


/** \brief Generate the product table header.
 *
 * This function generates the table header for the product table.
 *
 * \todo
 * TBD: should we have a signal so other plugins have a chance to tweak
 *      the header (one signal at the end would probably be enough.)
 *
 * @param {!jQuery} e  The body element where the header, table, and footer
 *                     can directly be added.
 * @param {!snapwebsites.eCommerceColumns} columns  The columns with all the necessary data.
 *
 * @private
 * @final
 */
snapwebsites.eCommerceCart.prototype.generateProductTableHeader_ = function(e, columns)
{
    var row = "<tr class='header'>",
        max = columns.size(),
        col,
        name,
        first = " first-column",
        display_name,
        i;

    for(i = 0; i < max; ++i)
    {
        col = columns.getColumnHeader(i);
        name = col.getName().replace(/:/g, '-');
        display_name = col.getDisplayName();

        row += "<th class='" + name + first + " first-row'>" + display_name + "</th>";

        first = "";
    }

    row += "</tr>";

    e.append(row);
};


/** \brief Generate the product table footer.
 *
 * This function generates the table footer for the product table.
 *
 * It is likely that you'd want to override this function if you want
 * to add "special" rows to the table. For example the shipping extension
 * adds a shipping line and the taxes extension adds a line with the
 * total amount of taxes.
 *
 * If you do change the number of columns, remember that you will have
 * to also override the generate_product_table_row() function to adjust
 * the table accordingly.
 *
 * \todo
 * Implement the signal to get other plugin features to add footer
 * entries as required, although we probably want that to happen
 * even ealier.
 *
 * @param {!jQuery} e  The body element where the header, table, and footer
 *                     can directly be added.
 * @param {!snapwebsites.eCommerceColumns} columns  The price of the product.
 * @param {Array.<snapwebsites.eCommerceProductFeatureBase>} footer_features  A reference to an array of features.
 *
 * @private
 * @final
 */
snapwebsites.eCommerceCart.prototype.generateProductTableFooter_ = function(e, columns, footer_features)
{
    var row = "",
        number_of_columns = columns.size(),
        columns_to_total = columns.getColumnIndex("ecommerce::total"), // already includes a '... - 1' since it is a position (index), not a count
        more_columns = number_of_columns - columns_to_total,
        net_grand_total = this.getTotalCosts(),
        max = footer_features.length,
        i,
        old_length,
        name,
        result = [];

    for(i = 0; i < max; ++i)
    {
        old_length = result.length;
        footer_features[i].generateFooter(columns, result, net_grand_total);
        if(old_length < result.length)
        {
            name = footer_features[i].getFeatureName().replace(/:/g, '-');
            do
            {
                // keep the name so the following features can determine
                // what each total is from
                result[old_length].name = name;

                // TBD: right now we are fine but it could be that
                //      some features would want to tweak previous
                //      results... (i.e. total is larger than X so
                //      reduce costs Y...)
                //
                // here we always create 2 columns, the first which spans
                // all the columns except the last on the right
                row += "<tr class='" + name + "-footer cart-row-footer'><td class='cart-row-footer-label first-column' colspan='"
                     + columns_to_total
                     + "'>"
                     + result[i].label
                     + "</td><td class='cart-row-footer-costs'>"
                     + result[i].costs
                     + "</td>"
                     + (more_columns > 0 ? "<td class='cart-row-footer-extras' colspan='" + more_columns + "'></td>" : "")
                     + "</tr>";

                ++old_length;
            }
            while(old_length < result.length);
        }
    }

    // row should never be empty here
    if(row)
    {
        e.append(row);
    }
};


/** \brief Generate the product table row.
 *
 * This function generates the table rows, one row per product.
 *
 * \todo
 * TBD: should we have a signal so other plugins have a chance to tweak
 *      the rows (one signal at the end would probably be enough.)
 *
 * @param {!jQuery} e  The body element where the rows can directly be added.
 * @param {!snapwebsites.eCommerceColumns} columns  The columns with all the necessary data.
 *
 * @private
 * @final
 */
snapwebsites.eCommerceCart.prototype.generateProductTableEmpty_ = function(e, columns)
{
    var number_of_columns = columns.size(),
        columns_to_total = columns.getColumnIndex("ecommerce::total"), // already includes a '... - 1' since it is a position (index), not a count
        more_columns = number_of_columns - columns_to_total,
        row = "<tr class='empty-cart-row'><td class='empty-cart-cell first-column' colspan='"
                + (columns_to_total + 1)
                + "'>&mdash; Your cart is currently empty &mdash;</td>"
            + (more_columns > 0 ? "<td class='cart-row-footer-extras' colspan='" + more_columns + "'></td>" : "")
                + "</tr>";

    e.append(row);
};


/** \brief Generate the product table row.
 *
 * This function generates the table rows, one row per product.
 *
 * \todo
 * TBD: should we have a signal so other plugins have a chance to tweak
 *      the rows (one signal at the end would probably be enough.)
 *
 * @param {!jQuery} e  The body element where the rows can directly be added.
 * @param {!number} row_index  The index of the product being added.
 * @param {!snapwebsites.eCommerceColumns} columns  The columns with all the necessary data.
 *
 * @private
 * @final
 */
snapwebsites.eCommerceCart.prototype.generateProductTableRow_ = function(e, row_index, columns)
{
    // note: the odd and even look inverted, that is because the
    //       first line is considered to be 1 but row_index is 0
    var row = "<tr class='cart-row "
            + (row_index & 1 ? "even" : "odd")
            + "'>",
        max = columns.size(),
        cell,
        name,
        value,
        first = " first-column",
        i;

    for(i = 0; i < max; ++i)
    {
        cell = columns.getColumnData(row_index, i);
        name = cell.getName().replace(/:/g, '-');
        value = cell.getValue();

        row += "<td class='" + name + first + "'>" + value + "</td>";

        first = "";
    }

    row += "</tr>";

    e.append(row);
};


/** \brief The server access timer needs the POST data.
 *
 * This function generates the POST data to be sent to the server.
 * This includes all the cart products.
 *
 * @param {string} request_name  The name of the request receiving this call.
 * @param {snapwebsites.ServerAccess} server_access  The server access object
 *                                                   to fill with our data.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.serverAccessTimerReady = function(request_name, server_access)
{
    var max = this.products_.length,
        guid,
        quantity,
        product_type,
        xml = "<?xml version='1.0'?><cart>",
        i;

    for(i = 0; i < max; ++i)
    {
        product_type = this.products_[i].getProductType();
        guid = product_type.getProperty("ecommerce::guid");
        quantity = this.products_[i].getQuantity();
        xml += "<product guid='" + guid + "' q='" + quantity + "'>";
        // TODO: we need to also handle the the attributes
        xml += "</product>";
    }

    xml += "</cart>";

    // TODO: I have to append "...?a=view" to the Query String in a rather unsafe manner
    server_access.setURI(snapwebsites.castToString(jQuery("link[rel='page-uri']").attr("href"), "casting href of the page-uri link to a string in snapwebsites.eCommerceCart.serverAccessTimerReady()") + "?a=view");
    server_access.setData({ ecommerce__cart_products: xml });
};



/** \brief Snap eCommerceProductFeatureBasic constructor.
 *
 * Initialize the basic product feature which is implemented internally.
 *
 * Other features can be added by adding new plugins and registering
 * the feature with the cart registerProductFeature() function.
 *
 * \code
 *  class eCommerceProductFeatureBasic extends eCommerceProductFeatureBase
 *  {
 *      eCommerceProductFeatureBasic();
 *
 *      virtual function getFeatureName() : String;
 *      virtual function setupColumnHeaders(columns: eCommerceColumns) : Void;
 *      virtual function setupColumnFooters(columns: eCommerceColumns, index: Number) : Void;
 *      virtual function setupColumns(product: eCommerceProductBase, in out columns: eCommerceColumns, index: Number) : Void;
 *      virtual function generateFooter(columns: eCommerceColumns, in out result: Object, net_grand_total: Number) : Void;
 *      virtual function getColumnDependencies() : String;
 *  };
 * \endcode
 *
 * @return {snapwebsites.eCommerceProductFeatureBasic} The newly created object.
 *
 * @constructor
 * @extends {snapwebsites.eCommerceProductFeatureBase}
 * @struct
 */
snapwebsites.eCommerceProductFeatureBasic = function()
{
    snapwebsites.eCommerceProductFeatureBasic.superClass_.constructor.call(this);

    return this;
};


/** \brief eCommerceProductFeatureBasic inherits the eCommerceProductFeatureBase interface.
 *
 * The basic feature is an implementation of the eCommerceProductFeatureBase
 * which offers the basics of the cart feature for a product.
 *
 * This includes the simple entries in a cart: designation, quantity, price,
 * total, total number of items, and grand total. Other features are defined
 * in various other ecommerce related plugins.
 */
snapwebsites.inherits(snapwebsites.eCommerceProductFeatureBasic, snapwebsites.eCommerceProductFeatureBase);


/** \brief Retrieve the name of this product feature.
 *
 * This function returns the product feature name. It is used whenever
 * you register the feature in the Cart object.
 *
 * @return {string}  The name of this cart product feature as a string.
 */
snapwebsites.eCommerceProductFeatureBasic.prototype.getFeatureName = function() // virtual
{
    return "ecommerce::basic";
};


/** \brief Setup the basic feature column headers.
 *
 * The basic feature adds the default column headers. This includes:
 *
 * \li Line #
 * \li Description
 * \li Quantity
 * \li Price
 * \li Total
 *
 * @param {!snapwebsites.eCommerceColumns} columns  The columns to setup with
 * header information.
 */
snapwebsites.eCommerceProductFeatureBasic.prototype.setupColumnHeaders = function(columns) // virtual
{
    // TODO: Translations
    columns.addColumnHeader("ecommerce::no", "#", "");
    columns.addColumnHeader("ecommerce::description", "Description", "");
    columns.addColumnHeader("ecommerce::quantity", "Quantity", "");
    columns.addColumnHeader("ecommerce::price", "Price", "");
    columns.addColumnHeader("ecommerce::total", "Total", "");
    columns.addColumnHeader("ecommerce::delete", "", ""); // no name for that column, we actually remove borders too!
};


/** \brief Setup various counters in the last row of data.
 *
 * This row is generally used to define a set of entries such as the
 * total number of items being purchased and the grand total before
 * adding taxes and shipping.
 *
 * The basic feature adds the total number of items in the
 * ecommerce::quantity column and the total costs in the
 * ecommerce::total column.
 *
 * Other features have the ability to remove those totals. This is
 * particularly useful if you have a cart with item quantities that
 * do not all match the same unit (i.e. you should not add meters
 * and millimeters as such, you'd have to convert the meters in
 * millimeters first--or vice versa--so the total makes some
 * sense.)
 *
 * \warning
 * This function is expected to be called after all the products were
 * added to the cart. It appends a row to the existing cart.
 *
 * @param {!snapwebsites.eCommerceColumns} columns  The columns where
 * footer information is to be added.
 * @param {number} index  The row index for this product entry.
 */
snapwebsites.eCommerceProductFeatureBasic.prototype.setupColumnFooters = function(columns, index) // virtual
{
    var total_quantity = snapwebsites.eCommerceCartInstance.getTotalQuantity(),
        total_costs = snapwebsites.eCommerceCartInstance.getTotalCosts();

    columns.addColumnData(index, "ecommerce::quantity", total_quantity);
    columns.addColumnData(index, "ecommerce::total", total_costs);
};


/** \brief Setup the basic feature columns.
 *
 * The basic feature adds the content of the default columns.
 *
 * This includes:
 *
 * \li Line #
 * \li Description
 * \li Quantity
 * \li Price
 * \li Total
 *
 * @param {snapwebsites.eCommerceProductBase} product  The product for which the columns are being setup.
 * @param {!snapwebsites.eCommerceColumns} columns  The columns were we want to save the product data.
 * @param {number} index  The row index for this product entry.
 */
snapwebsites.eCommerceProductFeatureBasic.prototype.setupColumns = function(product, columns, index) // virtual
{
    var quantity = product.getQuantity(),
        price = product.getPrice(),
        total = Math.round(quantity * price * 100.0) / 100.0,
        name;

    columns.addColumnData(index, "ecommerce::no", index + 1);

    columns.addColumnData(index, "ecommerce::description",
            "<a class='cart-product-description' href='"
            + product.getProductType().getProperty("ecommerce::guid")
            + "'>" + product.getDescription() + "</a>");

    // TODO: generate a unique name!
    name = "quantity";

    columns.addColumnData(index, "ecommerce::quantity",
            // add an editor widget and make it immediately active
            "<div field_type='line-edit' field_name='"
                        + name
                        + "' class='snap-editor editable line-edit immediate"
                        + (index == 0 ? " auto-focus" : "")
                        + " " + name
                        + "' spellcheck='false'>"
                 + "<div class='span-editor-background zordered'>"
                   + "<div class='snap-editor-background-content'>"
                     + "quantity"
                   + "</div>"
                 + "</div>"
                 // TODO: look into getting tabindex in there
                 + "<div name='"
                        + name
                        + "' class='editor-content no-toolbar'"
                        + " title='Enter the number of items you want. Put zero to remove this item from your cart.'"
                        + " minlength='1' maxlength='10'>"
                   + quantity
                 + "</div>"
                 // we could also add the help <div> ?
               + "</div>");
    // TODO: we need to get the editor widget functional too...

    columns.addColumnData(index, "ecommerce::price", price);
    columns.addColumnData(index, "ecommerce::total", total);
    columns.addColumnData(index, "ecommerce::delete", "<a class='cart-delete-product' href='/delete' guid='"
                + product.getProductType().getProperty("ecommerce::guid")
                + "'><img src='/images/ecommerce/button-delete-red-48x48.png' title='Delete' width='16' height='16'/></a>");
};


/** \brief Add the Grand Total row at the bottom of the cart.
 *
 * The basic feature adds the Grand Total row at the very bottom of the
 * cart. The footer features order ensures that the basic feature be
 * last.
 *
 * @param {!snapwebsites.eCommerceColumns} columns  The columns were we want to save the product data.
 * @param {Object} result  The result object.
 * @param {number} net_grand_total  The net grand total to purchase all the products in the cart.
 */
snapwebsites.eCommerceProductFeatureBasic.prototype.generateFooter = function(columns, result, net_grand_total) // virtual
{
    var max = result.length,
        i,
        grand_total = net_grand_total;

    for(i = 0; i < max; ++i)
    {
        grand_total += result[i].costs;
    }

    result.push({
            label: "Grand Total: ",
            costs: grand_total
        });
};


/** \brief List of dependencies of the basic feature.
 *
 * This function is overridden because the default dependency list is
 * "ecommerce::basic", which is this feature and we just do not want
 * to have this feature to depend on itself.
 *
 * @return {!string}  The list of dependencies, here an empty string.
 */
snapwebsites.eCommerceProductFeatureBasic.prototype.getColumnDependencies = function() // virtual
{
    return "";
};



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.eCommerceCartInstance = new snapwebsites.eCommerceCart();
        snapwebsites.eCommerceCartInstance.registerProductFeature(new snapwebsites.eCommerceProductFeatureBasic());
    });

// vim: ts=4 sw=4 et
