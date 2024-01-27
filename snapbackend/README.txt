In order to add admin rights to a user, run the following command:

  snapbackend -d --action permissions::makeroot -p ROOT_USER_EMAIL=user@domain.tld

If a user has trouble creating their account, we can remove the "new user"
link and then create another link that makes the user a certain type
such as admin or editor.

In Cassandra, look for links with that user like so:

    $ cqlsh --request-timeout=3600 --keyspace snap_websites <ip>
    cqlsh> use snap_websites;     # this is only if you want to change keyspace
    cqlsh> select blobastext(key), blobastext(column1), blobastext(value) from links;

You are likely to find out that there are too many links for the command above
to be useful. To find specific links, you want to query against `column1`.
For example, to find links about a given user:

    cqlsh> SELECT BLOBASTEXT(key), BLOBASTEXT(column1), BLOBASTEXT(value)
             FROM links
            WHERE column1 >= TEXTASBLOB('https://exdox.com/user/4056#')
              AND column1 < TEXTASBLOB('https://exdox.com/user/4056$') ALLOW FILTERING;

Note: links are <uri>#<name> hence the use of the '#' and '$'.

`snapbackend` can be used to run commands against our cassandra database.
Here are some examples:

    # request backend to do a clean up of all links
    snapbackend [--config snapserver.conf] [website-url] \
         --action links::cleanuplinks

    # create a link
    snapbackend [website-url] \
         [--config snapserver.conf] \
         --action links::createlink \
         --param SOURCE_LINK_NAME=users::author \
                 SOURCE_LINK=http://csnap.example.com/ \
                 DESTINATION_LINK_NAME=users::authored_pages \
                 DESTINATION_LINK=http://csnap.example.com/user/1 \
                 'LINK_MODE=1,*'

    # delete one specific link between two pages
    snapbackend [website-url] \
         [--config snapserver.conf] \
         --action links::deletelink \
         --param SOURCE_LINK_NAME=users::author \
                 SOURCE_LINK=/ \
                 DESTINATION_LINK_NAME=users::authored_pages \
                 DESTINATION_LINK=/user/1 \
                 'LINK_MODE=1,*'

    # delete all links named users::author in this page
    snapbackend [website-url] \
         [--config snapserver.conf] \
         --action links::deletelink \
         --param SOURCE_LINK_NAME=users::author \
                 SOURCE_LINK=/ \
                 LINK_MODE=1

To test on Android, one can download a .iso and run that in a virtualbox
window;

  http://www.android-x86.org/download

State manage (boost::signal2 and how to make it work right)

  http://cs.stackexchange.com/questions/27801/what-is-the-fastest-way-to-a-publish-subscribe-pattern
  doc/plan_interference_e_language.pdf
  http://en.wikipedia.org/wiki/Petri_net
  http://en.wikipedia.org/wiki/State_transition_system

vim: ts=2 sw=2 et
