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
  uint32_t nCsma = 100;  // CSMA Node의 총 갯수 = nCsma + 1

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  // command를 추가한 다음 거기에 Program Arguments를 추가

  cmd.Parse (argc,argv);

  if (verbose)  // verbose를 true로 세팅하였으므로 log를 출력시킴
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  nCsma = nCsma == 0 ? 1 : nCsma;  // nCsma의 값이 0이면 1을 대입하고, 아니면 변화 없음

  NodeContainer p2pNodes;
  p2pNodes.Create (2);  // P2P Node를 2개 생성 (Node(0), Node(1))

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));  // p2pNodes에 있는 Node(1)을 csmaNodes하고 연결
  csmaNodes.Create (nCsma);  // nCsma의 수만큼 CSMA Node를 생성
                             // (nCsma의 값이 100이므로 CSMA Node를 100개 생성)

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  // DataRate은 5Mbps로 Delay는 2ms로 설정하여 Point-To-Point 연결을 정의

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);  // 생성했던 2개의 P2P Node에 device를 각각 install함

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  // DataRate은 100Mbps로 Delay는 6560ns로 설정하여 CSMA 연결을 정의

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);  // 생성했던 101개의 CSMA Node에 device를 각각 install함

  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0));
  stack.Install (csmaNodes);
  // P2P Node의 Node(0)하고 4개의 CSMA Node에 각각 stack을 install함

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);
  // device가 install된 2개의 P2P Node에 IP 주소를 지정
  // (10.1.1.1, 10.1.1.2)

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);
  // device가 install된 101개의 CSMA Node에 IP 주소를 지정
  // (10.1.2.1, 10.1.2.2, 10.1.2.3, ... , 10.1.2.101)

  UdpEchoServerHelper echoServer (9);  // 포트번호가 9인 UDP Echo Server를 생성

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  // CSMA Node의 Node(3) 에다가 Server를 install함
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  // 모든 Server Application이 1초에서 10초동안 실행됨

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);  // CSMA Node 중 Node(3)의 IP주소를 갖고 포트번호가 9인 UDP Echo Client를 생성
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));  // simulation에서 packet을 보내는 최대의 양: 1
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));  // packet 간의 시간적 간격: 1초
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));  // packet의 사이즈: 1024 bytes

  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  // P2P Node의 Node(0) 에다가 Client를 install함
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  // 모든 Client Application이 2초에서 10초동안 실행됨

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // 각 Node들이 직접적으로 global route manager와 통신을 하게 됨
  // (라우터를 특별히 설정하지 않아도 복잡한 작업도 수행하게 해줌)

  pointToPoint.EnablePcapAll ("second");  // pcap 출력을 위한 코드를 삽입
  csma.EnablePcap ("second", csmaDevices.Get (1), true);  // CSMA 관련 pcap을 추가

  Simulator::Run ();
  Simulator::Destroy ();
  // 시뮬레이터를 생성 및 소멸시킴
  // (simulation이 끝나면, simulation을 위해 생성했던 객체들을 모두 제거함)

  return 0;
}
