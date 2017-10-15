#!/bin/sh
# Test the cxpath tool

set -e

TMP=/tmp
PATH=$PATH:`cd ..; pwd`/BUILD/snapwebsites/src

cat >$TMP/test.xml <<EOF
<?xml version="1.0"?>
<snap>
  <header>
    <metadata>
      <robotstxt>
        <noindex/>
        <nofollow/>
      </robotstxt>
      <sendmail>
        <param name="from">no-reply@m2osw.com</param>
        <param name="user-first-name">Alexis</param>
        <param name="user-last-name">Wilke</param>
        <param name="to">alexis@mail.example.com</param>
        <hidden name="not-a-param">ignore</hidden>
      </sendmail>
    </metadata>
    <statistics name="not-a-param">
      <value class="sonic">0.3</value>
      <value class="lumen">0.21</value>
      <value class="sonic">1.01</value>
      <value class="sonic">0.7</value>
      <value class="electromagnetic">0.855</value>
      <value class="lumen">0.32</value>
      <value class="sonic">0.05</value>
      <value class="electromagnetic">7.3</value>
      <value class="sonic">0.09</value>
      <value class="lumen">0.34</value>
      <value class="sonic">0.045</value>
      <value class="electromagnetic">2.01</value>
      <value class="sonic">0.62</value>
    </statistics>
    <hidden name="not-a-param">ignore</hidden>
  </header>
  <page>
    <date>
      <created format="K&quot;7">2013-11-27</created>
      <hidden name="not-a-date">ignore</hidden>
    </date>
  </page>
</snap>
EOF

echo "*** Get root"
cxpath -c -p '/' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get sendmail if a param is named to"
cxpath -c -p '/snap/header/metadata/sendmail/param[@name = "to"]/..' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get ancestors of sendmail"
cxpath -c -p '/snap/header/metadata/sendmail/ancestor::*' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get descendants of sendmail"
cxpath -c -p '/snap/header/metadata/sendmail/descendant::*' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get sendmail param named user-first-name or user-last-name (two full paths)"
cxpath -c -p '/snap/header/metadata/sendmail/param[@name = "user-first-name"]|/snap/header/metadata/sendmail/param[@name = "user-last-name"]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get sendmail param named user-first-name or user-last-name ('or' operator)"
cxpath -c -p '/snap/header/metadata/sendmail/param[@name = "user-first-name" or @name = "user-last-name"]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get following siblings of the param named user-first-name"
cxpath -c -p '/snap/header/metadata/sendmail/param[@name = "user-first-name"]/following-sibling::*' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get preceding siblings of the param named user-first-name"
cxpath -c -p '/snap/header/metadata/sendmail/param[@name = "user-first-name"]/preceding-sibling::*' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get param number two"
cxpath -c -p '/snap/header/metadata/sendmail/param[6 div 3]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get param number three"
cxpath -c -p '/snap/header/metadata/sendmail/param[6 idiv 2]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get param number four twice"
cxpath -c -p '/snap/header/metadata/sendmail/param[2 * 2] | /snap/header/metadata/sendmail/param[2 + 2]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get even positioned param's"
cxpath -c -p '/snap/header/metadata/sendmail/param[position() mod 2 = 0]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get odd positioned param's"
cxpath -c -p '/snap/header/metadata/sendmail/param[position() mod 2 = 1]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get the last and one before last params using last()"
cxpath -c -p '/snap/header/metadata/sendmail/param[position() = last()] | /snap/header/metadata/sendmail/param[position() = last()-1]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Test fn:true() as the predicate value"
cxpath -c -p '/snap/header/metadata/sendmail/param[fn:true()]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Test fn:false() as the predicate value"
cxpath -c -p '/snap/header/metadata/sendmail/param[fn:false()]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Test count() function"
cxpath -c -p '/snap/header/metadata/sendmail/param[count(/snap/header/metadata/sendmail/param) div 2]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get params except number two"
cxpath -c -p '/snap/header/metadata/sendmail/param[not(6 div 3 = position())]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get third param using round()"
cxpath -c -p '/snap/header/metadata/sendmail/param[round(6 div 2.2)]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get second param using floor()"
cxpath -c -p '/snap/header/metadata/sendmail/param[floor(6 div 2.2)]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get third param using ceiling()"
cxpath -c -p '/snap/header/metadata/sendmail/param[ceiling(6 div 2.8)]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get third param using string-length()"
cxpath -c -p '/snap/header/metadata/sendmail/param[string-length("abc")]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** List the lumen entries"
cxpath -c -p '/snap/header/statistics/value[@class="lumen"]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get third param using sum() of lumen class"
cxpath -c -p '/snap/header/metadata/sendmail/param[sum(/snap/header/statistics/value[@class="lumen"]) * 4]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get third param using sum() and ceiling() of sonic class"
cxpath -c -p '/snap/header/metadata/sendmail/param[ceiling(sum(/snap/header/statistics/value[@class="sonic"]))]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get third param using avg() of electromagnetic class"
cxpath -c -p '/snap/header/metadata/sendmail/param[avg(/snap/header/statistics/value[@class="electromagnetic"])]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get first param using min() and ceiling() of electromagnetic class"
cxpath -c -p '/snap/header/metadata/sendmail/param[ceiling(min(/snap/header/statistics/value[@class="electromagnetic"]))]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get second param using max() of electromagnetic class"
cxpath -c -p '/snap/header/metadata/sendmail/param[max(/snap/header/statistics/value[@class="electromagnetic"]) idiv 3]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get all params using empty()"
cxpath -c -p '/snap/header/metadata/sendmail/param[empty(/snap/does-not-exist)]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get nothing using empty()"
cxpath -c -p '/snap/header/metadata/sendmail/param[empty(/snap/header)]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get a node testing \" inside a string"
cxpath -c -p '/snap/page/date/created[@format = "K""7"]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get all the hidden nodes"
cxpath -c -p '//hidden' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get the hidden nodes marked as not-a-param"
cxpath -c -p '//hidden[@name = "not-a-param"]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get any node marked as not-a-param"
cxpath -c -p '//*[@name = "not-a-param"]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Get any node with a non empty name"
cxpath -c -p '//*[@name]' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

echo "*** Find a tag named sendmail and return its children named hidden"
cxpath -c -p '//sendmail/hidden' -o $TMP/test.xpath
cxpath -r -x $TMP/test.xpath $TMP/test.xml

# This still doesn't work... it should though
#echo "*** Get any node that include a noindex child"
#cxpath -c -p '//robotstxt[./noindex]' -o $TMP/test.xpath
#cxpath -r -x $TMP/test.xpath $TMP/test.xml

# vim: ts=2 sw=2 et
