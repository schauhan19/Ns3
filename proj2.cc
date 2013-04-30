/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Proj 1");

int main (int argc, char *argv[])
{

    // Set up Variables
    double latency = 2.0; // 2ms daly
    uint64_t dataRate = 5000000; //5Mbps data rate
    double interval = 0.02;   //Inter Packet Arrival Time

    /******** Note quite sure what this does********/
    CommandLine cmd;
    cmd.AddValue("latency", "P2P link Latency in Milliseconds",latency);
    cmd.Parse(argc,argv);
    /*******************************************/


    //Create Nodes
    //Nodes 4 and 5 are the bridging Nodees
    NodeContainer nodes;
    nodes.Create (6);

    //Left side
    NodeContainer left1 = NodeContainer( nodes.Get(0), nodes.Get(4));
    NodeContainer left2 = NodeContainer( nodes.Get(1), nodes.Get(4));
    //Right side
    NodeContainer right1 = NodeContainer( nodes.Get(2), nodes.Get(5));
    NodeContainer right2 = NodeContainer( nodes.Get(3), nodes.Get(5));
    //Bridge
    NodeContainer bridge = NodeContainer(nodes.Get(4), nodes.Get(5));


    //Stack
    InternetStackHelper stack;
    stack.Install (nodes);


    //Create Channels for each connection
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate",DataRateValue( DataRate (dataRate) ));
    pointToPoint.SetChannelAttribute ("Delay", TimeValue( MilliSeconds (latency) ));

    NetDeviceContainer devLeft1 = pointToPoint.Install (left1);
    NetDeviceContainer devLeft2 = pointToPoint.Install (left2);
    NetDeviceContainer devRight1= pointToPoint.Install (right1);
    NetDeviceContainer devRight2= pointToPoint.Install (right2);
    NetDeviceContainer devBridge= pointToPoint.Install (bridge);
    

    //Give everyone IP Addresses. Each connection wiill have a different Subnet
    Ipv4AddressHelper ipAdd;
    ipAdd.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iLeft1 = ipAdd.Assign (devLeft1);

    ipAdd.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer iLeft2 = ipAdd.Assign (devLeft2);

    ipAdd.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer iBridge = ipAdd.Assign(devBridge);

    ipAdd.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer iRight1 = ipAdd.Assign(devRight1);

    ipAdd.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer iRight2 = ipAdd.Assign(devRight2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables(); // For more than a single link


    //Connections and stuff

    /// CHANGE STUFF BELOW=============

    //Create Applications 
    //nodes.Get(1) == Server
    uint16_t port1 = 9;
    uint16_t port2 = 80;
    UdpEchoServerHelper echoServer (port1);
    UdpEchoServerHelper echoServer2 (port2);

    ApplicationContainer serverApps;
    serverApps = echoServer.Install (nodes.Get(1));    
    serverApps = echoServer2.Install(nodes.Get(1));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (12.0));


    //Client application to send from node 0 to 1
    uint32_t maxPacketSize = 1024;
    Time interPacketTime = Seconds(interval);
    uint32_t maxPacketCount = 5;

    UdpEchoClientHelper echoClient (interfaces.GetAddress (1), port1);
    UdpEchoClientHelper echoClient2 (interfaces.GetAddress (1), port2);

    echoClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    echoClient.SetAttribute ("Interval", TimeValue (interPacketTime));
    echoClient.SetAttribute ("PacketSize", UintegerValue (maxPacketSize));

    echoClient2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    echoClient2.SetAttribute ("Interval", TimeValue (interPacketTime));
    echoClient2.SetAttribute ("PacketSize", UintegerValue (maxPacketSize));

    ApplicationContainer clientApps;
    clientApps = echoClient.Install (nodes.Get (0));
    clientApps = echoClient2.Install(nodes.Get (0));
    clientApps.Start (Seconds (1.0));
    clientApps.Stop (Seconds (11.0));

    //Tracing Probably not needed
    AsciiTraceHelper ascii;
    pointToPoint.EnableAscii(ascii.CreateFileStream ("./scratch/lab1Res/lab1Trace.tr") ,devices);
    pointToPoint.EnablePcap("proj1",devices,false);

    //Calculating ThroughPut
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();



    std::cout<<"======LATENCY ="<<latency<<"=========="<<std::endl;

    Simulator::Stop (Seconds(12.0));
    Simulator::Run ();
    
    monitor->CheckForLostPackets ();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i){
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        if ((t.sourceAddress=="10.1.1.1" && t.destinationAddress == "10.1.1.2")){
            std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
            std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
            std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
        }
    }

    monitor->SerializeToXmlFile("./scratch/lab1Res/lab1.flowmon", true, true);
    Simulator::Destroy ();
    return 0;
}
