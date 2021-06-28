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
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 100;  // CSMA Node�� �� ���� = nCsma + 1

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  // command�� �߰��� ���� �ű⿡ Program Arguments�� �߰�

  cmd.Parse (argc,argv);

  if (verbose)  // verbose�� true�� �����Ͽ����Ƿ� log�� ��½�Ŵ
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  nCsma = nCsma == 0 ? 1 : nCsma;  // nCsma�� ���� 0�̸� 1�� �����ϰ�, �ƴϸ� ��ȭ ����

  NodeContainer p2pNodes;
  p2pNodes.Create (2);  // P2P Node�� 2�� ���� (Node(0), Node(1))

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));  // p2pNodes�� �ִ� Node(1)�� csmaNodes�ϰ� ����
  csmaNodes.Create (nCsma);  // nCsma�� ����ŭ CSMA Node�� ����
                             // (nCsma�� ���� 100�̹Ƿ� CSMA Node�� 100�� ����)

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  // DataRate�� 5Mbps�� Delay�� 2ms�� �����Ͽ� Point-To-Point ������ ����

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);  // �����ߴ� 2���� P2P Node�� device�� ���� install��

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  // DataRate�� 100Mbps�� Delay�� 6560ns�� �����Ͽ� CSMA ������ ����

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);  // �����ߴ� 101���� CSMA Node�� device�� ���� install��

  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0));
  stack.Install (csmaNodes);
  // P2P Node�� Node(0)�ϰ� 4���� CSMA Node�� ���� stack�� install��

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);
  // device�� install�� 2���� P2P Node�� IP �ּҸ� ����
  // (10.1.1.1, 10.1.1.2)

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);
  // device�� install�� 101���� CSMA Node�� IP �ּҸ� ����
  // (10.1.2.1, 10.1.2.2, 10.1.2.3, ... , 10.1.2.101)

  UdpEchoServerHelper echoServer (9);  // ��Ʈ��ȣ�� 9�� UDP Echo Server�� ����

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  // CSMA Node�� Node(3) ���ٰ� Server�� install��
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  // ��� Server Application�� 1�ʿ��� 10�ʵ��� �����

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);  // CSMA Node �� Node(3)�� IP�ּҸ� ���� ��Ʈ��ȣ�� 9�� UDP Echo Client�� ����
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));  // simulation���� packet�� ������ �ִ��� ��: 1
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));  // packet ���� �ð��� ����: 1��
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));  // packet�� ������: 1024 bytes

  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  // P2P Node�� Node(0) ���ٰ� Client�� install��
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  // ��� Client Application�� 2�ʿ��� 10�ʵ��� �����

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // �� Node���� ���������� global route manager�� ����� �ϰ� ��
  // (����͸� Ư���� �������� �ʾƵ� ������ �۾��� �����ϰ� ����)

  pointToPoint.EnablePcapAll ("second");  // pcap ����� ���� �ڵ带 ����
  csma.EnablePcap ("second", csmaDevices.Get (1), true);  // CSMA ���� pcap�� �߰�

  Simulator::Run ();
  Simulator::Destroy ();
  // �ùķ����͸� ���� �� �Ҹ��Ŵ
  // (simulation�� ������, simulation�� ���� �����ߴ� ��ü���� ��� ������)

  return 0;
}
