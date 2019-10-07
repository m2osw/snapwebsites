
# Commands

Basic commands we can send to the backend.

## `CONNECT`

Connect using a user name/password. This is useful only if you place
the service on a separate computer.

## `DISCONNECT`

Explicitly close a connection with the database.

## `SET`

Add or update a key/value pair.

## `INSERT`

Add a new key/value pair. If the key already exists, the command fails.

## `UPDATE`

Update a key/value pair. If the key does not exist, the command fails.

## `GET`

Retrieve the value specifying a key.

## `DELETE`

Get rid of a given key.

## `LOCK`

Lock the specified cell, row, table, or context.

Since we have a lock feature, we can offer such a locking mechanism in
our database. After all, it makes sense to have a lock feature too.

## `PING`

Make sure the connection is live.

## `LISTEN`

Listen to changes to a list of keys and statistics.

## Reply: `ACKNOWLEDGEMENT`

Most commands reply with an acknowledgement reply.

## Reply: `CHANGE`

Message from a `LISTEN`

## Reply: `STATUS`

Status messages from the server. These only happen when a `LISTEN` message
was sent first.

The `STATUS` includes statistics about the server so that way we can better
manage certain things like to which server to send data because it is not
as high load as another system.

## Reply: `BYE`

The server decided to terminate this connection.


