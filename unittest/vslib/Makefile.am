AM_CXXFLAGS = $(SAIINC) -I$(top_srcdir)/meta -I$(top_srcdir)/vslib -I$(top_srcdir)/lib -I/usr/include/libnl3

bin_PROGRAMS = tests testslibsaivs

LDADD_GTEST = -L/usr/src/gtest -lgtest -lgtest_main

tests_SOURCES = main.cpp \
				TestBuffer.cpp \
				TestContextConfigContainer.cpp \
				TestCorePortIndexMap.cpp \
				TestCorePortIndexMapContainer.cpp \
				TestCorePortIndexMapFileParser.cpp \
				TestEventPayloadNetLinkMsg.cpp \
				TestEventPayloadPacket.cpp \
				TestEventQueue.cpp \
				TestFdbInfo.cpp \
				TestSaiAttrWrap.cpp \
				TestLaneMap.cpp \
				TestLaneMapContainer.cpp \
				TestLaneMapFileParser.cpp \
				TestMACsecAttr.cpp \
				TestMACsecEgressFilter.cpp \
				TestMACsecForwarder.cpp \
				TestMACsecIngressFilter.cpp \
				TestMACsecFilterStateGuard.cpp \
				TestNetMsgRegistrar.cpp \
				TestRealObjectIdManager.cpp \
				TestResourceLimiter.cpp \
				TestResourceLimiterContainer.cpp \
				TestResourceLimiterParser.cpp \
				TestSaiUnittests.cpp \
				TestSelectableFd.cpp \
				TestSignal.cpp \
				TestSwitchConfigContainer.cpp \
				TestTrafficForwarder.cpp \
				TestHostInterfaceInfo.cpp \
				TestTrafficFilterPipes.cpp \
				TestSwitchConfig.cpp \
				TestSwitchContainer.cpp \
				TestSwitch.cpp \
				TestSwitchMLNX2700.cpp \
				TestSwitchNvdaMBF2H536C.cpp \
				TestSwitchBCM56850.cpp \
				TestSwitchBCM81724.cpp \
				TestSwitchStateBaseMACsec.cpp \
				TestMACsecManager.cpp \
				TestSwitchStateBase.cpp \
				TestVirtualSwitchSaiInterface.cpp

tests_CXXFLAGS = $(DBGFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS_COMMON) -fno-access-control
tests_LDADD = $(LDADD_GTEST) $(top_srcdir)/vslib/libSaiVS.a -lhiredis -lswsscommon -lnl-genl-3 -lnl-nf-3 -lnl-route-3 -lnl-3 \
			  -lpthread -L$(top_srcdir)/meta/.libs -lsaimetadata -lsaimeta -lzmq $(CODE_COVERAGE_LIBS)

testslibsaivs_SOURCES =	main_libsaivs.cpp \
				test_sai_vs_acl.cpp \
				test_sai_vs_bfd.cpp \
				test_sai_vs_bmtor.cpp \
				test_sai_vs_bridge.cpp \
				test_sai_vs_buffer.cpp \
				test_sai_vs_counter.cpp \
				test_sai_vs_dash_vip.cpp \
				test_sai_vs_dash_pa_validation.cpp \
				test_sai_vs_dash_vnet.cpp \
				test_sai_vs_dash_outbound_routing.cpp \
				test_sai_vs_dash_outbound_ca_to_pa.cpp \
				test_sai_vs_dash_inbound_routing.cpp \
				test_sai_vs_dash_eni.cpp \
				test_sai_vs_dash_direction_lookup.cpp \
				test_sai_vs_dash_acl.cpp \
				test_sai_vs_debug_counter.cpp \
				test_sai_vs_dtel.cpp \
				test_sai_vs_fdb.cpp \
				test_sai_vs_generic_programmable.cpp \
				test_sai_vs_ipmc.cpp \
				test_sai_vs_l2mc.cpp \
				test_sai_vs_l2mcgroup.cpp \
				test_sai_vs_lag.cpp \
				test_sai_vs_ipmc_group.cpp \
				test_sai_vs_macsec.cpp \
				test_sai_vs_isolation_group.cpp \
				test_sai_vs_interfacequery.cpp \
				test_sai_vs_mcastfdb.cpp \
				test_sai_vs_mirror.cpp \
				test_sai_vs_mpls.cpp \
				test_sai_vs_nat.cpp \
				test_sai_vs_hash.cpp \
				test_sai_vs_neighbor.cpp \
				test_sai_vs_nexthop.cpp \
				test_sai_vs_nexthopgroup.cpp \
				test_sai_vs_port.cpp \
				test_sai_vs_qosmap.cpp \
				test_sai_vs_policer.cpp \
				test_sai_vs_queue.cpp \
				test_sai_vs_route.cpp \
				test_sai_vs_router_interface.cpp \
				test_sai_vs_router_rpfgroup.cpp \
				test_sai_vs_router_samplepacket.cpp \
				test_sai_vs_schedulergroup.cpp \
				test_sai_vs_scheduler.cpp \
				test_sai_vs_srv6.cpp \
				test_sai_vs_switch.cpp \
				test_sai_vs_system_port.cpp \
				test_sai_vs_tam.cpp \
				test_sai_vs_tunnel.cpp \
				test_sai_vs_stp.cpp \
				test_sai_vs_udf.cpp \
				test_sai_vs_virtual_router.cpp \
				test_sai_vs_vlan.cpp \
				test_sai_vs_hostif.cpp \
				test_sai_vs_wred.cpp \
				test_sai_vs_ars.cpp \
				test_sai_vs_ars_profile.cpp

testslibsaivs_CXXFLAGS = $(DBGFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS_COMMON)
testslibsaivs_LDADD = $(LDADD_GTEST) -L$(top_srcdir)/vslib/.libs -lsaivs -lhiredis -lswsscommon -lpthread -L$(top_srcdir)/meta/.libs -lsaimetadata -lsaimeta -lzmq $(CODE_COVERAGE_LIBS)

TESTS = testslibsaivs tests
