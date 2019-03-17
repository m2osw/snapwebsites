#!/bin/sh
#
# Text:
#      snaplock/tests/test_leaders.sh
#
# Description:
#      Test the snaplock daemon to make sure that it properly elects
#      daemons over time.
#
# Documentation:
#      What does the test do?
#
#      We added code in the debug version that allows us to give a specific
#      service name to various instances of the snaplock daemon. This allows
#      us to start any number of instances on a single computer, making it
#      much easier to stress test the tool without the need of a large cloud
#      setup which could be costly to most of us.
#
#      The system allows for (1) multiple snapcommunicators and (2) multiple
#      snaplock. Each snapcommunicator requires a different private IP address
#      so we create a setup that assigns one of your ethernet card many IP
#      addresses. Note that this is rather unconvential as far as Snap! is
#      concerned, but it would not too difficult to accomplish and we still
#      get a valid test which can simulate having around 200 nodes in your
#      cloud.
#
# License:
#      Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
# 
#      https://snapwebsites.org/
#      contact@m2osw.com
# 
#      Permission is hereby granted, free of charge, to any person obtaining a
#      copy of this software and associated documentation files (the
#      "Software"), to deal in the Software without restriction, including
#      without limitation the rights to use, copy, modify, merge, publish,
#      distribute, sublicense, and/or sell copies of the Software, and to
#      permit persons to whom the Software is furnished to do so, subject to
#      the following conditions:
#
#      The above copyright notice and this permission notice shall be included
#      in all copies or substantial portions of the Software.
#
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#


BASE_IP=10.22.0.0





# vim: ts=4 sw=4 et
