<?php

// This script is available to check all your machines connections.
// The script loads a ~/.config/snapwebsites/snap-machines.php
// with arrays of clusters you manage. Enter the name of the
// cluster to check and the result is a CVS with what the
// snapcommunicator is connected with on each on of your machines.
//

// Here is an example of a cluster definition named "cassandra":
//
// $cluster = array(
//    // My VPS at home
//    "cassandra" => array(
//        "snap1"      => array("public" => "192.168.2.91",    "private" => "172.16.0.1", ),
//        "cassandra1" => array("public" => "192.168.2.92",    "private" => "172.16.0.2", ),
//        "backend1"   => array("public" => "192.168.2.93",    "private" => "172.16.0.3", ),
//    ),
// );

$cluster_definition_file = getenv("HOME") . "/.config/snapwebsites/snap-machines.php";
if(!file_exists($cluster_definition_file))
{
    echo "error: the {$cluster_definition_file} file is missing.\n";
    echo "Please check this PHP script for additional information.\n";
    exit(1);
}

if(count($argv) != 2)
{
    echo "error: script expects one argument, the name of the cluster to be checked.\n";
    exit(1);
}

require($cluster_definition_file);

$cluster_name = $argv[1];

if(!array_key_exists($cluster_name, $clusters))
{
    echo "error: {$cluster_name} is not defined in your cluster definitions. Misspelled?\n";
    exit(1);
}

$cluster_definition = $clusters[$cluster_name];

function parse_netstat_output($name, $netstat)
{
    global $cluster_definition;

    foreach($netstat as $line)
    {
        $columns = preg_split("/\s+/", $line);
        if(count($columns) != 6
        || $columns[0] != "tcp")
        {
            echo "warning: found a non \"tcp\" line: $line\n";
            continue;
        }

//echo "$name => $columns[3]\t$columns[4]\t$columns[5]\n";

        if($columns[5] == "LISTEN")
        {
            if($columns[4] != "0.0.0.0:*")
            {
                echo "error: expected column 5 to be 0.0.0.0:* for a LISTEN, got {$columns[4]} instead.\n";
                exit(1);
            }
            if($columns[3] == "127.0.0.1:4040")
            {
                if(isset($cluster_definition[$name]["localhost_listen"]))
                {
                    echo "error: LISTEN on localhost found twice.\n";
                    exit(1);
                }
                $cluster_definition[$name]["localhost_listen"] = true;
            }
            else
            {
                $ip_port = preg_split("/:/", $columns[3]);
                if(count($ip_port) != 2)
                {
                    echo "error: expected column 4 to include an IP and a port, got {$columns[3]} instead.\n";
                    exit(1);
                }
                if($ip_port[1] != 4040)
                {
                    echo "error: expected column 4 to have port 4040, found {$ip_port[1]} instead.\n";
                    exit(1);
                }
                if($ip_port[0] != $cluster_definition[$name]["private"])
                {
                    echo "error: expected column 4 private IP address to be {$cluster_definition[$name]["private"]}, found {$ip_port[0]} instead (LISTEN connection).\n";
                    exit(1);
                }
                if(isset($cluster_definition[$name]["private_listen"]))
                {
                    echo "error: LISTEN on private network found twice.\n";
                    exit(1);
                }
                $cluster_definition[$name]["private_listen"] = true;
            }
        }
        else if($columns[5] == "ESTABLISHED")
        {
            $ip_port = preg_split("/:/", $columns[3]);
            if(count($ip_port) != 2)
            {
                echo "error: column 3 is expected to have an IP and a port, got {$columns[3]} instead.\n";
                exit(1);
            }
            if($ip_port[0] == "127.0.0.1")
            {
                // this is a local connection, we just count those
                //
                if($ip_port[1] == 4040)
                {
                    // a process connected to us
                    if(isset($cluster_definition[$name]["local_connection"]))
                    {
                        $cluster_definition[$name]["local_connection"]++;
                    }
                    else
                    {
                        $cluster_definition[$name]["local_connection"] = 1;
                    }
                }
                else
                {
                    // a process back connection from us
                    if(isset($cluster_definition[$name]["local_back_connection"]))
                    {
                        $cluster_definition[$name]["local_back_connection"]++;
                    }
                    else
                    {
                        $cluster_definition[$name]["local_back_connection"] = 1;
                    }
                }
            }
            else
            {
                // this is an external connection, we want to mark that on
                // both ends: the other connection cluster definition and
                // our own definition
                //
                if($ip_port[0] != $cluster_definition[$name]["private"])
                {
                    echo "error: expected column 4 private IP address to be {$cluster_definition[$name]["private"]}, found {$ip_port[0]} instead (ESTABLISHED connection).\n";
                    exit(1);
                }
                $other_ip_port = preg_split("/:/", $columns[4]);
                if(count($other_ip_port) != 2)
                {
                    echo "error: column 5 is expected to have an IP and a port, got {$columns[4]} instead.\n";
                    exit(1);
                }
                $other_name = null;
                foreach($cluster_definition as $this_name => $other_definition)
                {
                    if($other_definition["private"] == $other_ip_port[0])
                    {
                        // found the IP!
                        $other_name = $this_name;
                        break;
                    }
                }
                if(!$other_name)
                {
                    echo "error: column 5 is expectedto have a known IP, got {$columns[4]} instead.\n";
                    exit(1);
                }

                if($ip_port[1] == 4040)
                {
                    if(isset($cluster_definition[$name][$other_name])
                    || isset($cluster_definition[$other_name][$name . "-back"]))
                    {
                        echo "error: already found the {$name} and {$other_name} combo.\n";
                        exit(1);
                    }
                    $cluster_definition[$name][$other_name] = $other_ip_port[1];
                    $cluster_definition[$other_name][$name . "-back"] = $ip_port[1];
                }
                else
                {
                    // the "-back" is on the other definition
                    //
                    if(isset($cluster_definition[$name][$other_name])
                    || isset($cluster_definition[$other_name][$name . "-back"]))
                    {
                        echo "error: already found the {$name} and {$other_name} combo.\n";
                        exit(1);
                    }
                    $cluster_definition[$name][$other_name] = $other_ip_port[1];
                    $cluster_definition[$other_name][$name . "-back"] = $ip_port[1];
                }
            }
        }
        else
        {
            echo "warning: column 5 of {$name} is currently {$columns[5]} instead of LISTEN or ESTABLISHED (IPs: ${columns[3]} and ${columns[4]}).\n";
            // continue, this is likely a TIME_WAIT
        }
    }
}

foreach($cluster_definition as $name => $definition)
{
    $ip = $definition["public"];

    $netstat = array();
    $return_value = 0;
    exec("ssh " . $ip . " netstat -a64n | grep 4040", $netstat, $return_value);

    if($return_value != 0)
    {
        echo "error: could not connection to {$ip}.\n";
        exit(1);
    }

    parse_netstat_output($name, $netstat);
}

//var_dump($cluster_definition);


// TODO:
// change output to use HTML instead of CSV so we can format the data and
// view it in a browser which is the general environment used in Snap!...
//
// then we could integrate this script in snapmanager.cgi and have a status
// of the cluster from that interface, except that ssh won't work inter-node
// at this point... but we should be able to add a netstat.php script and
// use that to get the expected output!
//
// so something like this:
//
// instead of "ssh netstat -a64n | grep 4040"
//
// we would do "wget http://[private ip]/netstat.php"
// (setup to be implemented then, i.e. we may want to use HTTPS only and a
// port other than port 80 so we can open it safely to all our private nodes
// but not the outside world)
//

$f = fopen("cluster.csv", "w");

fwrite($f, "Computer Name,Public IP,Private IP,LISTEN");

foreach($cluster_definition as $name => $definition)
{
    fwrite($f, "," . $name);
}

fwrite($f, "\n");

foreach($cluster_definition as $name => $definition)
{
    fwrite($f, $name . "," . $definition["public"] . "," . $definition["private"]);

    $listen_public = isset($definition["public"]);
    $listen_private = isset($definition["private"]);
    if($listen_public && $listen_private)
    {
        fwrite($f, ",valid");
    }
    else if($listen_public)
    {
        fwrite($f, ",private missing");
    }
    else if($listen_private)
    {
        fwrite($f, ",public missing");
    }
    else
    {
        fwrite($f, ",no listen");
    }

    foreach($cluster_definition as $sub_name => $sub_definition)
    {
        $sub_def = isset($definition[$sub_name]);
        $sub_def_back = isset($definition[$sub_name . "-back"]);

        if($sub_def && $sub_def_back)
        {
            if($definition[$sub_name] != $definition[$sub_name . "-back"])
            {
                echo "error: port mistach in {$name} / {$sub_name}, found {$definition[$sub_name]} and {$definition[$sub_name . "-back"]}.";
                exit(1);
            }
            fwrite($f, "," . $definition[$sub_name]);
        }
        else if($sub_def)
        {
            fwrite($f, ",back missing");
        }
        else if($sub_def_back)
        {
            fwrite($f, ",definition missing");
        }
        else
        {
            if($name == $sub_name)
            {
                // we don't connect to self, that's normal
                //
                fwrite($f, ",-");
            }
            else
            {
                fwrite($f, ",missing");
            }
        }
    }

    fwrite($f, "\n");
}

fclose($f);

// vim: ts=4 sw=4 et
