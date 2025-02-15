#include "Meta.h"
#include "MockMeta.h"
#include "MetaTestSaiInterface.h"

#include <arpa/inet.h>
#include <net/ethernet.h>

#include <gtest/gtest.h>

#include <memory>

using namespace saimeta;

template<typename L>
static bool compare_lists(L la, L lb)
{
    // SWSS_LOG_ENTER(); // disabled

    if (la.count != lb.count) {
        return false;
    }

    return std::equal(la.list, la.list + la.count, lb.list);
}

static bool operator==(sai_ip_address_t a, sai_ip_address_t b)
{
    if (a.addr_family != b.addr_family) {
        return false;
    }

    switch (a.addr_family) {
        case SAI_IP_ADDR_FAMILY_IPV4:
            return a.addr.ip4 == b.addr.ip4;
        case SAI_IP_ADDR_FAMILY_IPV6:
            return memcmp(a.addr.ip6, b.addr.ip6, sizeof(a.addr.ip6)) == 0;
        default:
            return false;
    }
}

static bool operator==(sai_ip_prefix_t a, sai_ip_prefix_t b)
{
    if (a.addr_family != b.addr_family) {
        return false;
    }

    switch (a.addr_family) {
        case SAI_IP_ADDR_FAMILY_IPV4:
            return a.addr.ip4 == b.addr.ip4 && a.mask.ip4 == b.mask.ip4;
        case SAI_IP_ADDR_FAMILY_IPV6:
            return memcmp(a.addr.ip6, b.addr.ip6, sizeof(a.addr.ip6)) == 0 &&
                memcmp(a.mask.ip6, b.mask.ip6, sizeof(a.mask.ip6)) == 0;
        default:
            return false;
    }
}

static bool operator==(sai_u16_range_t a, sai_u16_range_t b)
{
    return (a.min == b.min) && (a.max == b.max);
}

static sai_object_id_t create_switch(Meta &m)
{
    SWSS_LOG_ENTER();

    sai_object_id_t oid;
    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
    attr.value.booldata = true;

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create(SAI_OBJECT_TYPE_SWITCH, &oid, SAI_NULL_OBJECT_ID, 1, &attr));

    return oid;
}

static sai_object_id_t create_counter(Meta &m, sai_object_id_t switchid)
{
    SWSS_LOG_ENTER();

    sai_object_id_t oid;

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create(SAI_OBJECT_TYPE_COUNTER, &oid, switchid, 0, nullptr));

    return oid;
}

static void remove_counter(Meta &m, sai_object_id_t counter)
{
    SWSS_LOG_ENTER();

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove((sai_object_type_t)SAI_OBJECT_TYPE_COUNTER, counter));
}

static sai_object_id_t create_vnet(Meta &m, sai_object_id_t switchid, uint32_t vni)
{
    SWSS_LOG_ENTER();

    sai_object_id_t oid;
    sai_attribute_t attr;

    attr.id = SAI_VNET_ATTR_VNI;
    attr.value.u32 = vni;

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_VNET, &oid, switchid, 1, &attr));

    return oid;
}

static void remove_vnet(Meta &m, sai_object_id_t vnet)
{
    SWSS_LOG_ENTER();

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove((sai_object_type_t)SAI_OBJECT_TYPE_VNET, vnet));
}

static sai_object_id_t create_eni(Meta &m, sai_object_id_t switchid, sai_object_id_t vnet)
{
    SWSS_LOG_ENTER();

    sai_object_id_t oid;
    sai_attribute_t attr;

    attr.id = SAI_ENI_ATTR_VNET_ID;
    attr.value.oid = vnet;

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_ENI, &oid, switchid, 1, &attr));

    return oid;
}

static void remove_eni(Meta &m, sai_object_id_t eni)
{
    SWSS_LOG_ENTER();

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove((sai_object_type_t)SAI_OBJECT_TYPE_ENI, eni));
}

TEST(Meta, quad_dash_direction_lookup)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    sai_direction_lookup_entry_t entry;

    entry.switch_id = switchid;
    entry.vni = 1;

    std::vector<sai_attribute_t> attrs;

    attr.id = SAI_DIRECTION_LOOKUP_ENTRY_ATTR_ACTION;
    attr.value.s32 = SAI_DIRECTION_LOOKUP_ENTRY_ACTION_SET_OUTBOUND_DIRECTION;
    attrs.push_back(attr);

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create(&entry, (uint32_t)attrs.size(), attrs.data()));

    attr.id = SAI_DIRECTION_LOOKUP_ENTRY_ATTR_ACTION;
    attr.value.s32 = SAI_DIRECTION_LOOKUP_ENTRY_ACTION_SET_OUTBOUND_DIRECTION;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set(&entry, &attr));
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get(&entry, (uint32_t)attrs.size(), attrs.data()));
    EXPECT_EQ(attrs[0].value.s32, SAI_DIRECTION_LOOKUP_ENTRY_ACTION_SET_OUTBOUND_DIRECTION);

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove(&entry));
}

TEST(Meta, bulk_dash_direction_lookup)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t entries_count = 2;
    const uint32_t entry_attrs_count = 1;

    sai_attribute_t attrs0[entry_attrs_count] = {
        {.id = SAI_DIRECTION_LOOKUP_ENTRY_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_DIRECTION_LOOKUP_ENTRY_ACTION_SET_OUTBOUND_DIRECTION}},
    };

     sai_attribute_t attrs1[entry_attrs_count] = {
        {.id = SAI_INBOUND_ROUTING_ENTRY_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_DIRECTION_LOOKUP_ENTRY_ACTION_SET_OUTBOUND_DIRECTION}},
    };

    const sai_attribute_t *attr_list[entries_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[entries_count] = {entry_attrs_count, entry_attrs_count};
    sai_status_t statuses[entries_count] = {};

    sai_direction_lookup_entry_t entries[entries_count] = {
        { .switch_id = switchid, .vni = 10},
        { .switch_id = switchid, .vni = 20},
    };

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate(entries_count, entries, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove(entries_count, entries, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }
}

TEST(Meta, quad_dash_eni_ether_address_map_entry)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    sai_object_id_t vnet = create_vnet(m, switchid, 100);
    sai_object_id_t eni = create_eni(m, switchid, vnet);

    sai_eni_ether_address_map_entry_t entry = { .switch_id = switchid, .address = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 }};

    std::vector<sai_attribute_t> attrs;
    attr.id = SAI_ENI_ETHER_ADDRESS_MAP_ENTRY_ATTR_ENI_ID;
    attr.value.oid = eni;
    attrs.push_back(attr);

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create(&entry, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get(&entry, (uint32_t)attrs.size(), attrs.data()));
    EXPECT_EQ(attrs[0].value.oid, eni);

    attr.id = SAI_ENI_ETHER_ADDRESS_MAP_ENTRY_ATTR_ENI_ID;
    attr.value.oid = eni;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set(&entry, &attr));
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get(&entry, (uint32_t)attrs.size(), attrs.data()));
    EXPECT_EQ(attrs[0].value.oid, eni);

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove(&entry));
    remove_eni(m, eni);
    remove_vnet(m, vnet);
}

TEST(Meta, bulk_dash_eni_ether_address_map_entry)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t entries_count = 2;
    const uint32_t entry_attrs_count = 1;

    sai_object_id_t vnet0 = create_vnet(m, switchid, 100);
    sai_object_id_t eni0 = create_eni(m, switchid, vnet0);

    sai_object_id_t vnet1 = create_vnet(m, switchid, 200);
    sai_object_id_t eni1 = create_eni(m, switchid, vnet1);

    sai_attribute_t attrs0[entry_attrs_count] = {
        {.id = SAI_ENI_ETHER_ADDRESS_MAP_ENTRY_ATTR_ENI_ID, .value = (sai_attribute_value_t){.oid = eni0}},
    };

     sai_attribute_t attrs1[entry_attrs_count] = {
        {.id = SAI_ENI_ETHER_ADDRESS_MAP_ENTRY_ATTR_ENI_ID, .value = (sai_attribute_value_t){.oid = eni1}},
    };

    const sai_attribute_t *attr_list[entries_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[entries_count] = {entry_attrs_count, entry_attrs_count};
    sai_status_t statuses[entries_count] = {};

    sai_eni_ether_address_map_entry_t entries[entries_count] = {
        { .switch_id = switchid, .address = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 }},
        { .switch_id = switchid, .address = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x06 }},
    };

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate(entries_count, entries, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove(entries_count, entries, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    remove_eni(m, eni0);
    remove_eni(m, eni1);
    remove_vnet(m, vnet0);
    remove_vnet(m, vnet1);
}

TEST(Meta, quad_dash_eni)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    sai_ip_address_t uip4, uip6;
    uip4.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.1", &uip4.addr.ip4);
    uip6.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "100::ffff:ffff:ffff:ffff", &uip6.addr.ip6);

    sai_object_id_t vnet = create_vnet(m, switchid, 101);

    std::vector<sai_attribute_t> attrs;

    attr.id = SAI_ENI_ATTR_VNET_ID;
    attr.value.oid = vnet;
    attrs.push_back(attr);

    attr.id = SAI_ENI_ATTR_ADMIN_STATE;
    attr.value.booldata = true;
    attrs.push_back(attr);

    attr.id = SAI_ENI_ATTR_VM_VNI;
    attr.value.u32 = 123;
    attrs.push_back(attr);

    attr.id = SAI_ENI_ATTR_CPS;
    attr.value.u32 = 10;
    attrs.push_back(attr);

    attr.id = SAI_ENI_ATTR_PPS;
    attr.value.u32 = 20;
    attrs.push_back(attr);

    attr.id = SAI_ENI_ATTR_FLOWS;
    attr.value.u32 = 30;
    attrs.push_back(attr);

    attr.id = SAI_ENI_ATTR_VM_UNDERLAY_DIP;
    attr.value.ipaddr = uip4;
    attrs.push_back(attr);

    sai_object_id_t eni;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_ENI, &eni, switchid, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get((sai_object_type_t)SAI_OBJECT_TYPE_ENI, eni, (uint32_t)attrs.size(), attrs.data()));
    EXPECT_EQ(attrs[0].value.oid, vnet);
    EXPECT_TRUE(attrs[1].value.booldata);
    EXPECT_EQ(attrs[2].value.u32, 123);
    EXPECT_EQ(attrs[3].value.u32, 10);
    EXPECT_EQ(attrs[4].value.u32, 20);
    EXPECT_EQ(attrs[5].value.u32, 30);
    EXPECT_EQ(attrs[6].value.ipaddr.addr.ip4, uip4.addr.ip4);

    attr.id = SAI_ENI_ATTR_CPS;
    attr.value.u32 = 10;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set((sai_object_type_t)SAI_OBJECT_TYPE_ENI, eni, &attr));
    attr.id = SAI_ENI_ATTR_VM_UNDERLAY_DIP;
    attr.value.ipaddr = uip6;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set((sai_object_type_t)SAI_OBJECT_TYPE_ENI, eni, &attr));

    remove_eni(m, eni);
    remove_vnet(m, vnet);
}

TEST(Meta, bulk_dash_eni)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t enis_count = 2;
    const uint32_t eni_attrs_count = 6;

    sai_object_id_t vnet0 = create_vnet(m, switchid, 101);
    sai_object_id_t vnet1 = create_vnet(m, switchid, 102);

    sai_ip_address_t ipaddr0, ipaddr1;
    ipaddr0.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.1", &ipaddr0.addr.ip4);

    ipaddr1.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.1.1", &ipaddr1.addr.ip4);

    sai_attribute_t attrs0[] = {
        {.id = SAI_ENI_ATTR_VNET_ID, .value = (sai_attribute_value_t){.oid = vnet0}},
        {.id = SAI_ENI_ATTR_ADMIN_STATE, .value = (sai_attribute_value_t){.booldata = true}},
        {.id = SAI_ENI_ATTR_VM_VNI, .value = (sai_attribute_value_t){.u32 = 123}},
        {.id = SAI_ENI_ATTR_CPS, .value = (sai_attribute_value_t){.u32 = 10}},
        {.id = SAI_ENI_ATTR_FLOWS, .value = (sai_attribute_value_t){.u32 = 20}},
        {.id = SAI_ENI_ATTR_VM_UNDERLAY_DIP, .value = (sai_attribute_value_t){.ipaddr = ipaddr0}},
    };

    sai_attribute_t attrs1[] = {
        {.id = SAI_ENI_ATTR_VNET_ID, .value = (sai_attribute_value_t){.oid = vnet1}},
        {.id = SAI_ENI_ATTR_ADMIN_STATE, .value = (sai_attribute_value_t){.booldata = true}},
        {.id = SAI_ENI_ATTR_VM_VNI, .value = (sai_attribute_value_t){.u32 = 124}},
        {.id = SAI_ENI_ATTR_CPS, .value = (sai_attribute_value_t){.u32 = 11}},
        {.id = SAI_ENI_ATTR_FLOWS, .value = (sai_attribute_value_t){.u32 = 21}},
        {.id = SAI_ENI_ATTR_VM_UNDERLAY_DIP, .value = (sai_attribute_value_t){.ipaddr = ipaddr1}},
    };

    const sai_attribute_t *attr_list[enis_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[enis_count] = {eni_attrs_count, eni_attrs_count};
    sai_object_id_t enis[enis_count] = {};
    sai_status_t statuses[enis_count] = {};

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate((sai_object_type_t)SAI_OBJECT_TYPE_ENI, switchid, enis_count, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, enis, statuses));
    for (uint32_t i = 0; i < enis_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove((sai_object_type_t)SAI_OBJECT_TYPE_ENI, enis_count, enis, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < enis_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    remove_vnet(m, vnet0);
    remove_vnet(m, vnet1);
}

TEST(Meta, quad_dash_eni_acl)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    std::vector<sai_eni_attr_t> acl_attrs = {
        SAI_ENI_ATTR_INBOUND_V4_STAGE1_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_INBOUND_V4_STAGE2_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_INBOUND_V4_STAGE3_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_INBOUND_V4_STAGE4_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_INBOUND_V4_STAGE5_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_INBOUND_V6_STAGE1_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_INBOUND_V6_STAGE2_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_INBOUND_V6_STAGE3_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_INBOUND_V6_STAGE4_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_INBOUND_V6_STAGE5_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_OUTBOUND_V4_STAGE1_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_OUTBOUND_V4_STAGE2_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_OUTBOUND_V4_STAGE3_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_OUTBOUND_V4_STAGE4_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_OUTBOUND_V4_STAGE5_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_OUTBOUND_V6_STAGE1_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_OUTBOUND_V6_STAGE2_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_OUTBOUND_V6_STAGE3_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_OUTBOUND_V6_STAGE4_DASH_ACL_GROUP_ID,
        SAI_ENI_ATTR_OUTBOUND_V6_STAGE5_DASH_ACL_GROUP_ID,
    };

    std::vector<sai_attribute_t> attrs;
    std::vector<sai_object_id_t> acl_groups, acl_groups_new;
    for (auto at : acl_attrs) {
        sai_object_id_t acl_group;
        EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, &acl_group, switchid, 0, nullptr));
        acl_groups.push_back(acl_group);

        attr.id = at;
        attr.value.oid = acl_group;
        attrs.push_back(attr);

        EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, &acl_group, switchid, 0, nullptr));
        acl_groups_new.push_back(acl_group);
    }

    sai_object_id_t eni;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_ENI, &eni, switchid, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get((sai_object_type_t)SAI_OBJECT_TYPE_ENI, eni, (uint32_t)attrs.size(), attrs.data()));
    for (size_t i = 0; i < attrs.size(); i++) {
        attr.id = attrs[i].id;
        attr.value.oid = acl_groups_new[i];
        EXPECT_EQ(SAI_STATUS_SUCCESS, m.set((sai_object_type_t)SAI_OBJECT_TYPE_ENI, eni, &attr));
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get((sai_object_type_t)SAI_OBJECT_TYPE_ENI, eni, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove((sai_object_type_t)SAI_OBJECT_TYPE_ENI, eni));
}

TEST(Meta, quad_dash_vip)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    std::vector<sai_attribute_t> attrs;

    attr.id = SAI_VIP_ENTRY_ATTR_ACTION;
    attr.value.s32 = SAI_VIP_ENTRY_ACTION_ACCEPT;
    attrs.push_back(attr);

    sai_ip_address_t vip_addr;
    vip_addr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.1", &vip_addr.addr.ip4);

    sai_vip_entry_t vip = { .switch_id = switchid, .vip = vip_addr };
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create(&vip, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get(&vip, (uint32_t)attrs.size(), attrs.data()));
    EXPECT_EQ(attrs[0].value.s32, SAI_VIP_ENTRY_ACTION_ACCEPT);

    attr.id = SAI_ENI_ETHER_ADDRESS_MAP_ENTRY_ATTR_ENI_ID;
    attr.value.s32 = SAI_VIP_ENTRY_ACTION_ACCEPT;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set(&vip, &attr));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove(&vip));
}

TEST(Meta, bulk_dash_vip)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t vips_count = 2;
    const uint32_t vip_attrs_count = 1;

    sai_attribute_t attrs0[] = {
        {.id = SAI_VIP_ENTRY_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_VIP_ENTRY_ACTION_ACCEPT}},
    };

     sai_attribute_t attrs1[] = {
        {.id = SAI_VIP_ENTRY_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_VIP_ENTRY_ACTION_ACCEPT}},
    };

    const sai_attribute_t *attr_list[vips_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[vips_count] = {vip_attrs_count, vip_attrs_count};
    sai_status_t statuses[vips_count] = {};

    sai_ip_address_t vip_addr0, vip_addr1;
    vip_addr0.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.1", &vip_addr0.addr.ip4);
    vip_addr1.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "100::ffff:ffff:ffff:ffff", &vip_addr1.addr.ip6);

    sai_vip_entry_t vips[vips_count] = {
        {.switch_id = switchid, .vip = vip_addr0},
        {.switch_id = switchid, .vip = vip_addr1},
    };

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate(vips_count, vips, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < vips_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove(vips_count, vips, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < vips_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }
}

TEST(Meta, quad_dash_acl_group)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    std::vector<sai_attribute_t> attrs;

    attr.id = SAI_DASH_ACL_GROUP_ATTR_IP_ADDR_FAMILY;
    attr.value.s32 = SAI_IP_ADDR_FAMILY_IPV4;
    attrs.push_back(attr);

    sai_object_id_t acl_group;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, &acl_group, switchid, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, acl_group, (uint32_t)attrs.size(), attrs.data()));
    EXPECT_EQ(attrs[0].value.s32, SAI_IP_ADDR_FAMILY_IPV4);

    attr.id = SAI_DASH_ACL_GROUP_ATTR_IP_ADDR_FAMILY;
    attr.value.s32 = SAI_IP_ADDR_FAMILY_IPV6;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, acl_group, &attr));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, acl_group));
}

TEST(Meta, bulk_dash_acl_group)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t acls_count = 2;
    const uint32_t acl_attrs_count = 1;

    sai_attribute_t attrs0[acl_attrs_count] = {
        {.id = SAI_DASH_ACL_GROUP_ATTR_IP_ADDR_FAMILY, .value = (sai_attribute_value_t){.s32 = SAI_IP_ADDR_FAMILY_IPV4}},
    };

    sai_attribute_t attrs1[acl_attrs_count] = {
        {.id = SAI_DASH_ACL_GROUP_ATTR_IP_ADDR_FAMILY, .value = (sai_attribute_value_t){.s32 = SAI_IP_ADDR_FAMILY_IPV6}},
    };

    const sai_attribute_t *attr_list[acls_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[acls_count] = {acl_attrs_count, acl_attrs_count};
    sai_object_id_t acls[acls_count];
    sai_status_t statuses[acls_count] = {};

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, switchid, acls_count, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, acls, statuses));
    for (uint32_t i = 0; i < acls_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, acls_count, acls, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < acls_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }
}

TEST(Meta, quad_dash_acl_rule)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    sai_object_id_t group;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, &group, switchid, 0, nullptr));

    sai_object_id_t counter = create_counter(m, switchid);

    sai_ip_prefix_t ip_addr_list[2] = {};

    ip_addr_list[0].addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.1", &ip_addr_list[0].addr.ip4);
    inet_pton(AF_INET, "255.255.0.0", &ip_addr_list[0].mask.ip4);
    ip_addr_list[1].addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "100::ffff:ffff:ffff:ffff", &ip_addr_list[1].addr.ip6);
    inet_pton(AF_INET6, "ffff:fff0::", &ip_addr_list[1].mask.ip6);
    sai_ip_prefix_list_t ip_prefix_list0 = {.count = 2, .list = ip_addr_list};
    sai_ip_prefix_list_t ip_prefix_list1 = {.count = 1, .list = ip_addr_list};

    uint8_t protos[2] = {0xAA, 0xBB};
    sai_u8_list_t protos_list = {.count=2, .list = protos};

    sai_u16_range_t port_ranges[2] = {{.min = 10, .max = 20}, {.min = 30, .max = 40}};
    sai_u16_range_list_t port_ranges_list0 = {.count=2, .list = port_ranges};
    sai_u16_range_list_t port_ranges_list1 = {.count=1, .list = port_ranges};

    std::vector<sai_attribute_t> attrs;

    attr.id = SAI_DASH_ACL_RULE_ATTR_ACTION;
    attr.value.s32 = SAI_DASH_ACL_RULE_ACTION_PERMIT;
    attrs.push_back(attr);

    attr.id = SAI_DASH_ACL_RULE_ATTR_DASH_ACL_GROUP_ID;
    attr.value.oid = group;
    attrs.push_back(attr);

    attr.id = SAI_DASH_ACL_RULE_ATTR_DIP;
    attr.value.ipprefixlist = ip_prefix_list0;
    attrs.push_back(attr);

    attr.id = SAI_DASH_ACL_RULE_ATTR_SIP;
    attr.value.ipprefixlist = ip_prefix_list1;
    attrs.push_back(attr);

    attr.id = SAI_DASH_ACL_RULE_ATTR_PROTOCOL;
    attr.value.u8list = protos_list;
    attrs.push_back(attr);

    attr.id = SAI_DASH_ACL_RULE_ATTR_SRC_PORT;
    attr.value.u16rangelist = port_ranges_list0;
    attrs.push_back(attr);

    attr.id = SAI_DASH_ACL_RULE_ATTR_DST_PORT;
    attr.value.u16rangelist = port_ranges_list1;
    attrs.push_back(attr);

    attr.id = SAI_DASH_ACL_RULE_ATTR_COUNTER_ID;
    attr.value.oid = counter;
    attrs.push_back(attr);

    attr.id = SAI_DASH_ACL_RULE_ATTR_PRIORITY;
    attr.value.u32 = 1;
    attrs.push_back(attr);

    sai_object_id_t acl;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_RULE, &acl, switchid, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_RULE, acl, (uint32_t)attrs.size(), attrs.data()));
    EXPECT_EQ(attrs[0].value.s32, SAI_DASH_ACL_RULE_ACTION_PERMIT);
    EXPECT_EQ(attrs[1].value.oid, group);
    ASSERT_TRUE(compare_lists(attrs[2].value.ipprefixlist, ip_prefix_list0));
    ASSERT_TRUE(compare_lists(attrs[3].value.ipprefixlist, ip_prefix_list1));
    ASSERT_TRUE(compare_lists(attrs[4].value.u8list, protos_list));
    ASSERT_TRUE(compare_lists(attrs[5].value.u16rangelist, port_ranges_list0));
    ASSERT_TRUE(compare_lists(attrs[6].value.u16rangelist, port_ranges_list1));
    EXPECT_EQ(attrs[7].value.oid, counter);
    EXPECT_EQ(attrs[8].value.u32, 1);

    attr.id = SAI_DASH_ACL_RULE_ATTR_ACTION;
    attr.value.s32 = SAI_DASH_ACL_RULE_ACTION_DENY_AND_CONTINUE;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_RULE, acl, &attr));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_RULE, acl));
    remove_counter(m, counter);
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, group));
}

TEST(Meta, bulk_dash_acl_rule)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t acls_count = 2;
    const uint32_t acl_attrs_count = 9;

    sai_object_id_t counter0 = create_counter(m, switchid);
    sai_object_id_t counter1 = create_counter(m, switchid);

    sai_object_id_t group0;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, &group0, switchid, 0, nullptr));

    sai_object_id_t group1;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, &group1, switchid, 0, nullptr));

    sai_ip_prefix_t ip_prefix_arr0[2] = {};
    ip_prefix_arr0[0].addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.1", &ip_prefix_arr0[0].addr.ip4);
    inet_pton(AF_INET, "255.255.255.0", &ip_prefix_arr0[0].mask.ip4);
    ip_prefix_arr0[1].addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "100::ffff:ffff:ffff:ffff", &ip_prefix_arr0[1].addr.ip6);
    inet_pton(AF_INET6, "ffff::", &ip_prefix_arr0[1].mask.ip6);

    sai_ip_prefix_t ip_prefix_arr1[2] = {};
    ip_prefix_arr1[0].addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.2", &ip_prefix_arr1[0].addr.ip4);
    inet_pton(AF_INET, "255.255.0.0", &ip_prefix_arr1[0].mask.ip4);
    ip_prefix_arr1[1].addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "100::ffff:ffff:ffff:fffe", &ip_prefix_arr1[1].addr.ip6);
    inet_pton(AF_INET6, "ffff::", &ip_prefix_arr1[1].mask.ip6);

    sai_ip_prefix_list_t ip_prefix_list0 = {.count = 1, .list = ip_prefix_arr0};
    sai_ip_prefix_list_t ip_prefix_list1 = {.count = 1, .list = ip_prefix_arr1};

    uint8_t protos0[2] = {0xAA, 0xBB};
    uint8_t protos1[2] = {0xCC, 0xDD};
    sai_u8_list_t protos_list0 = {.count=2, .list = protos0};
    sai_u8_list_t protos_list1 = {.count=2, .list = protos1};

    sai_u16_range_t port_ranges0[2] = {{.min = 10, .max = 20}, {.min = 30, .max = 40}};
    sai_u16_range_t port_ranges1[2] = {{.min = 50, .max = 60}, {.min = 70, .max = 80}};
    sai_u16_range_list_t u16_range_list0 = {.count=2, .list = port_ranges0};
    sai_u16_range_list_t u16_range_list1 = {.count=2, .list = port_ranges1};

    sai_attribute_t attrs0[acl_attrs_count] = {
        {.id = SAI_DASH_ACL_RULE_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_DASH_ACL_RULE_ACTION_PERMIT_AND_CONTINUE}},
        {.id = SAI_DASH_ACL_RULE_ATTR_DASH_ACL_GROUP_ID, .value = (sai_attribute_value_t){.oid = group0}},
        {.id = SAI_DASH_ACL_RULE_ATTR_DIP, .value = (sai_attribute_value_t){.ipprefixlist = ip_prefix_list0}},
        {.id = SAI_DASH_ACL_RULE_ATTR_SIP, .value = (sai_attribute_value_t){.ipprefixlist = ip_prefix_list1}},
        {.id = SAI_DASH_ACL_RULE_ATTR_PROTOCOL, .value = (sai_attribute_value_t){.u8list = protos_list0}},
        {.id = SAI_DASH_ACL_RULE_ATTR_SRC_PORT, .value = (sai_attribute_value_t){.u16rangelist = u16_range_list0}},
        {.id = SAI_DASH_ACL_RULE_ATTR_DST_PORT, .value = (sai_attribute_value_t){.u16rangelist = u16_range_list1}},
        {.id = SAI_DASH_ACL_RULE_ATTR_COUNTER_ID, .value = (sai_attribute_value_t){.oid = counter0}},
        {.id = SAI_DASH_ACL_RULE_ATTR_PRIORITY, .value = (sai_attribute_value_t){.u32 = 1}},
    };

    sai_attribute_t attrs1[acl_attrs_count] = {
        {.id = SAI_DASH_ACL_RULE_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_DASH_ACL_RULE_ACTION_DENY}},
        {.id = SAI_DASH_ACL_RULE_ATTR_DASH_ACL_GROUP_ID, .value = (sai_attribute_value_t){.oid = group1}},
        {.id = SAI_DASH_ACL_RULE_ATTR_DIP, .value = (sai_attribute_value_t){.ipprefixlist = ip_prefix_list0}},
        {.id = SAI_DASH_ACL_RULE_ATTR_SIP, .value = (sai_attribute_value_t){.ipprefixlist = ip_prefix_list1}},
        {.id = SAI_DASH_ACL_RULE_ATTR_PROTOCOL, .value = (sai_attribute_value_t){.u8list = protos_list1}},
        {.id = SAI_DASH_ACL_RULE_ATTR_SRC_PORT, .value = (sai_attribute_value_t){.u16rangelist = u16_range_list1}},
        {.id = SAI_DASH_ACL_RULE_ATTR_DST_PORT, .value = (sai_attribute_value_t){.u16rangelist = u16_range_list0}},
        {.id = SAI_DASH_ACL_RULE_ATTR_COUNTER_ID, .value = (sai_attribute_value_t){.oid = counter1}},
        {.id = SAI_DASH_ACL_RULE_ATTR_PRIORITY, .value = (sai_attribute_value_t){.u32 = 2}},
    };

    const sai_attribute_t *attr_list[acls_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[acls_count] = {acl_attrs_count, acl_attrs_count};
    sai_object_id_t acls[acls_count];
    sai_status_t statuses[acls_count] = {};

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_RULE, switchid, acls_count, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, acls, statuses));
    for (uint32_t i = 0; i < acls_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_RULE, acls_count, acls, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < acls_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, group0));
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove((sai_object_type_t)SAI_OBJECT_TYPE_DASH_ACL_GROUP, group1));
    remove_counter(m, counter0);
    remove_counter(m, counter1);
}

TEST(Meta, quad_dash_vnet)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    sai_object_id_t vnet;
    attr.id = SAI_VNET_ATTR_VNI;
    attr.value.u32 = 10;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create((sai_object_type_t)SAI_OBJECT_TYPE_VNET, &vnet, switchid, 1, &attr));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get((sai_object_type_t)SAI_OBJECT_TYPE_VNET, vnet, 1, &attr));
    EXPECT_EQ(attr.value.u32, 10);

    attr.id = SAI_VNET_ATTR_VNI;
    attr.value.u32 = 20;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set((sai_object_type_t)SAI_OBJECT_TYPE_VNET, vnet, &attr));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove((sai_object_type_t)SAI_OBJECT_TYPE_VNET, vnet));
}

TEST(Meta, bulk_dash_vnet)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t vnets_count = 2;
    const uint32_t vnet_attrs_count = 1;

    sai_attribute_t attrs0[] = {
        {.id = SAI_VNET_ATTR_VNI, .value = (sai_attribute_value_t){.u32 = 10}},
    };

     sai_attribute_t attrs1[] = {
        {.id = SAI_VNET_ATTR_VNI, .value = (sai_attribute_value_t){.u32 = 20}},
    };

    const sai_attribute_t *attr_list[vnets_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[vnets_count] = {vnet_attrs_count, vnet_attrs_count};
    sai_object_id_t vnets[vnets_count];
    sai_status_t statuses[vnets_count] = {};

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate((sai_object_type_t)SAI_OBJECT_TYPE_VNET, switchid, vnets_count, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, vnets, statuses));
    for (uint32_t i = 0; i < vnets_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove((sai_object_type_t)SAI_OBJECT_TYPE_VNET, vnets_count, vnets, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < vnets_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }
}

TEST(Meta, quad_dash_inbound_routing_entry)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    sai_object_id_t vnet = create_vnet(m, switchid, 10);
    sai_object_id_t eni = create_eni(m, switchid, vnet);

    sai_ip_address_t sip, sip_mask;
    sip.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.1", &sip.addr.ip4);
    sip_mask.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "255.255.255.0", &sip_mask.addr.ip4);

    sai_inbound_routing_entry_t entry = { .switch_id = switchid, .eni_id = eni, .vni = 10, .sip = sip, .sip_mask = sip_mask, .priority = 1};

    std::vector<sai_attribute_t> attrs;
    attr.id = SAI_INBOUND_ROUTING_ENTRY_ATTR_ACTION;
    attr.value.s32 = SAI_INBOUND_ROUTING_ENTRY_ACTION_VXLAN_DECAP_PA_VALIDATE;
    attrs.push_back(attr);

    attr.id = SAI_INBOUND_ROUTING_ENTRY_ATTR_SRC_VNET_ID;
    attr.value.oid = vnet;
    attrs.push_back(attr);

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create(&entry, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get(&entry, (uint32_t)attrs.size(), attrs.data()));

    attr.id = SAI_INBOUND_ROUTING_ENTRY_ATTR_ACTION;
    attr.value.s32 = SAI_INBOUND_ROUTING_ENTRY_ACTION_VXLAN_DECAP;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set(&entry, &attr));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove(&entry));
    remove_eni(m, eni);
    remove_vnet(m, vnet);
}

TEST(Meta, bulk_dash_inbound_routing_entry)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t entries_count = 2;
    const uint32_t entry_attrs_count = 2;

    sai_object_id_t vnet0 = create_vnet(m, switchid, 101);
    sai_object_id_t eni0 = create_eni(m, switchid, vnet0);

    sai_object_id_t vnet1 = create_vnet(m, switchid, 102);
    sai_object_id_t eni1 = create_eni(m, switchid, vnet0);

    sai_ip_address_t sip0, sip_mask0;
    sai_ip_address_t sip1, sip_mask1;
    sip0.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.1", &sip0.addr.ip4);
    sip_mask0.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "255.255.255.0", &sip_mask0.addr.ip4);
    sip1.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.2", &sip1.addr.ip4);
    sip_mask1.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "255.255.0.0", &sip_mask1.addr.ip4);

    sai_attribute_t attrs0[entry_attrs_count] = {
        {.id = SAI_INBOUND_ROUTING_ENTRY_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_INBOUND_ROUTING_ENTRY_ACTION_VXLAN_DECAP}},
        {.id = SAI_INBOUND_ROUTING_ENTRY_ATTR_SRC_VNET_ID, .value = (sai_attribute_value_t){.oid = vnet0}},
    };

     sai_attribute_t attrs1[entry_attrs_count] = {
        {.id = SAI_INBOUND_ROUTING_ENTRY_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_INBOUND_ROUTING_ENTRY_ACTION_VXLAN_DECAP}},
        {.id = SAI_INBOUND_ROUTING_ENTRY_ATTR_SRC_VNET_ID, .value = (sai_attribute_value_t){.oid = vnet1}},
    };

    const sai_attribute_t *attr_list[entries_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[entries_count] = {entry_attrs_count, entry_attrs_count};
    sai_status_t statuses[entries_count] = {};

    sai_inbound_routing_entry_t entries[entries_count] = {
        { .switch_id = switchid, .eni_id = eni0, .vni = 10, .sip = sip0, .sip_mask = sip_mask0, .priority = 1},
        { .switch_id = switchid, .eni_id = eni1, .vni = 100, .sip = sip1, .sip_mask = sip_mask1, .priority = 2}
    };

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate(entries_count, entries, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove(entries_count, entries, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    remove_eni(m, eni0);
    remove_eni(m, eni1);
    remove_vnet(m, vnet0);
    remove_vnet(m, vnet1);
}

TEST(Meta, quad_dash_pa_validation)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    sai_object_id_t vnet = create_vnet(m, switchid, 10);

    sai_pa_validation_entry_t entry;
    entry.switch_id = switchid;
    entry.vnet_id = vnet;
    entry.sip.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.3.3.3", &entry.sip.addr.ip4);

    std::vector<sai_attribute_t> attrs;
    attr.id = SAI_PA_VALIDATION_ENTRY_ATTR_ACTION;
    attr.value.s32 = SAI_PA_VALIDATION_ENTRY_ACTION_PERMIT;
    attrs.push_back(attr);

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create(&entry, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get(&entry, (uint32_t)attrs.size(), attrs.data()));
    EXPECT_EQ(attrs[0].value.s32, SAI_PA_VALIDATION_ENTRY_ACTION_PERMIT);

    attr.id = SAI_PA_VALIDATION_ENTRY_ATTR_ACTION;
    attr.value.s32 = SAI_PA_VALIDATION_ENTRY_ACTION_PERMIT;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set(&entry, &attr));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove(&entry));
    remove_vnet(m, vnet);
}

TEST(Meta, bulk_dash_pa_validation)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t entries_count = 2;
    const uint32_t entry_attrs_count = 1;

    sai_object_id_t vnet0 = create_vnet(m, switchid, 10);
    sai_object_id_t vnet1 = create_vnet(m, switchid, 20);

    sai_ip_address_t sip0 = {};
    sai_ip_address_t sip1 = {};
    sip0.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    sip1.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET, "192.1.1.1", &sip0.addr.ip4);
    inet_pton(AF_INET6, "ffff::", &sip1.addr.ip6);

    sai_attribute_t attrs0[entry_attrs_count] = {
        {.id = SAI_PA_VALIDATION_ENTRY_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_PA_VALIDATION_ENTRY_ACTION_PERMIT}},
    };

     sai_attribute_t attrs1[entry_attrs_count] = {
        {.id = SAI_PA_VALIDATION_ENTRY_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_PA_VALIDATION_ENTRY_ACTION_PERMIT}},
    };

    const sai_attribute_t *attr_list[entries_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[entries_count] = {entry_attrs_count, entry_attrs_count};
    sai_status_t statuses[entries_count] = {};

    sai_pa_validation_entry_t entries[entries_count] = {
        { .switch_id = switchid, .vnet_id = vnet0, .sip = sip0},
        { .switch_id = switchid, .vnet_id = vnet1, .sip = sip1},
    };

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate(entries_count, entries, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove(entries_count, entries, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    remove_vnet(m, vnet0);
    remove_vnet(m, vnet1);
}

TEST(Meta, quad_dash_outbound_routing_entry)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_attribute_t attr;

    sai_object_id_t counter = create_counter(m, switchid);
    sai_object_id_t vnet = create_vnet(m, switchid, 101);
    sai_object_id_t eni = create_eni(m, switchid, vnet);

    sai_ip_address_t oip6;
    oip6.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "ffff::", &oip6.addr);

    sai_outbound_routing_entry_t entry0;
    entry0.switch_id = switchid;
    entry0.eni_id = eni;
    entry0.destination.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.1.0", &entry0.destination.addr.ip4);
    inet_pton(AF_INET, "255.255.255.0", &entry0.destination.mask.ip4);

    std::vector<sai_attribute_t> attrs;
    attr.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_ACTION;
    attr.value.u32 = SAI_OUTBOUND_ROUTING_ENTRY_ACTION_ROUTE_VNET;
    attrs.push_back(attr);
    attr.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_DST_VNET_ID;
    attr.value.oid = vnet;
    attrs.push_back(attr);
    attr.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_OVERLAY_IP;
    attr.value.ipaddr = oip6;
    attrs.push_back(attr);
    attr.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_COUNTER_ID;
    attr.value.oid = counter;
    attrs.push_back(attr);
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create(&entry0, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get(&entry0, (uint32_t)attrs.size(), attrs.data()));
    EXPECT_EQ(attrs[0].value.u32, SAI_OUTBOUND_ROUTING_ENTRY_ACTION_ROUTE_VNET);
    EXPECT_EQ(attrs[1].value.oid, vnet);
    EXPECT_EQ(attrs[2].value.ipaddr, oip6);
    EXPECT_EQ(attrs[3].value.oid, counter);

    attr.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_ACTION;
    attr.value.u32 = SAI_OUTBOUND_ROUTING_ENTRY_ACTION_ROUTE_DIRECT;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set(&entry0, &attr));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove(&entry0));

    remove_eni(m, eni);
    remove_vnet(m, vnet);
    remove_counter(m, counter);
}

TEST(Meta, bulk_dash_outbound_routing_entry)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t entries_count = 2;
    const uint32_t entry_attrs_count = 4;

    sai_object_id_t counter0 = create_counter(m, switchid);
    sai_object_id_t counter1 = create_counter(m, switchid);

    sai_ip_address_t oip4, oip6;
    oip4.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "1.2.3.4", &oip4.addr);
    oip6.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET6, "ffff::", &oip6.addr);

    sai_object_id_t vnet0 = create_vnet(m, switchid, 101);
    sai_object_id_t vnet1 = create_vnet(m, switchid, 102);
    sai_object_id_t eni0 = create_eni(m, switchid, vnet0);
    sai_object_id_t eni1 = create_eni(m, switchid, vnet1);

    sai_ip_prefix_t dst0 = {};
    sai_ip_prefix_t dst1 = {};
    dst0.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    dst1.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    inet_pton(AF_INET, "192.1.1.1", &dst0.addr.ip4);
    inet_pton(AF_INET, "255.255.255.0", &dst0.mask.ip4);
    inet_pton(AF_INET6, "fe80::886a:feff:fe31:bfe0", &dst1.addr.ip6);
    inet_pton(AF_INET6, "ffff:ffff::", &dst1.mask.ip6);

    sai_attribute_t attrs0[entry_attrs_count] = {
        {.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_OUTBOUND_ROUTING_ENTRY_ACTION_ROUTE_VNET}},
        {.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_DST_VNET_ID, .value = (sai_attribute_value_t){.oid = vnet0}},
        {.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_OVERLAY_IP, .value = (sai_attribute_value_t){.ipaddr = oip4}},
        {.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_COUNTER_ID, .value = (sai_attribute_value_t){.oid = counter0}},
    };

     sai_attribute_t attrs1[entry_attrs_count] = {
        {.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_ACTION, .value = (sai_attribute_value_t){.s32 = SAI_OUTBOUND_ROUTING_ENTRY_ACTION_ROUTE_VNET}},
        {.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_DST_VNET_ID, .value = (sai_attribute_value_t){.oid = vnet1}},
        {.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_OVERLAY_IP, .value = (sai_attribute_value_t){.ipaddr = oip6}},
        {.id = SAI_OUTBOUND_ROUTING_ENTRY_ATTR_COUNTER_ID, .value = (sai_attribute_value_t){.oid = counter1}},
    };

    const sai_attribute_t *attr_list[entries_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[entries_count] = {entry_attrs_count, entry_attrs_count};
    sai_status_t statuses[entries_count] = {};

    sai_outbound_routing_entry_t entries[entries_count] = {
        { .switch_id = switchid, .eni_id = eni0, .destination = dst0},
        { .switch_id = switchid, .eni_id = eni1, .destination = dst1},
    };

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate(entries_count, entries, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove(entries_count, entries, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    remove_eni(m, eni0);
    remove_eni(m, eni1);
    remove_vnet(m, vnet0);
    remove_vnet(m, vnet1);
    remove_counter(m, counter0);
    remove_counter(m, counter1);
}

TEST(Meta, quad_dash_outbound_ca_to_pa_entry)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    sai_object_id_t counter = create_counter(m, switchid);

    sai_ip_address_t uip4;
    uip4.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.1", &uip4.addr.ip4);

    sai_object_id_t vnet = create_vnet(m, switchid, 10);

    sai_outbound_ca_to_pa_entry_t entry;
    entry.switch_id = switchid;
    entry.dst_vnet_id = vnet;
    entry.dip.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.168.0.1", &entry.dip.addr.ip4);

    sai_attribute_t attr;
    std::vector<sai_attribute_t> attrs;

    attr.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_UNDERLAY_DIP;
    attr.value.ipaddr = uip4;
    attrs.push_back(attr);

    attr.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_OVERLAY_DMAC;
    sai_mac_t dmac = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };
    memcpy(attr.value.mac, dmac, 6);
    attrs.push_back(attr);

    attr.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_USE_DST_VNET_VNI;
    attr.value.booldata = true;
    attrs.push_back(attr);

    attr.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_COUNTER_ID;
    attr.value.oid = counter;
    attrs.push_back(attr);

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.create(&entry, (uint32_t)attrs.size(), attrs.data()));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.get(&entry, (uint32_t)attrs.size(), attrs.data()));
    EXPECT_EQ(attrs[0].value.ipaddr, uip4);
    EXPECT_EQ(memcmp(attrs[1].value.mac, dmac, sizeof(dmac)), 0);
    EXPECT_EQ(attrs[2].value.booldata, true);
    EXPECT_EQ(attrs[3].value.oid, counter);

    attr.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_USE_DST_VNET_VNI;
    attr.value.booldata = true;
    EXPECT_EQ(SAI_STATUS_SUCCESS, m.set(&entry, &attr));

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.remove(&entry));

    remove_vnet(m, vnet);
    remove_counter(m, counter);
}

TEST(Meta, bulk_dash_outbound_ca_to_pa_entry)
{
    Meta m(std::make_shared<MetaTestSaiInterface>());

    sai_object_id_t switchid = create_switch(m);

    const uint32_t entries_count = 2;
    const uint32_t entry_attrs_count = 4;

    sai_object_id_t counter0 = create_counter(m, switchid);
    sai_object_id_t counter1 = create_counter(m, switchid);

    sai_object_id_t vnet0 = create_vnet(m, switchid, 10);
    sai_object_id_t vnet1 = create_vnet(m, switchid, 20);

    sai_ip_address_t dip0 = {};
    sai_ip_address_t dip1 = {};
    sai_ip_address_t udip0 = {};
    sai_ip_address_t udip1 = {};
    dip0.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    dip1.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    udip0.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    udip1.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    inet_pton(AF_INET, "192.1.1.1", &dip0.addr.ip4);
    inet_pton(AF_INET6, "fe80::886a:feff:fe31:bfe0", &dip1.addr.ip6);
    inet_pton(AF_INET6, "fe80::886a:feff:fe31:bfe1", &udip0.addr.ip6);
    inet_pton(AF_INET, "192.1.1.2", &udip1.addr.ip4);

    sai_attribute_t attrs0[entry_attrs_count] = {
        {.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_UNDERLAY_DIP, .value = (sai_attribute_value_t){.ipaddr = udip0}},
        {.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_OVERLAY_DMAC, .value = (sai_attribute_value_t){.mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}}},
        {.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_USE_DST_VNET_VNI, .value = (sai_attribute_value_t){.booldata = true}},
        {.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_COUNTER_ID, .value = (sai_attribute_value_t){.oid = counter0}}
    };

     sai_attribute_t attrs1[entry_attrs_count] = {
        {.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_UNDERLAY_DIP, .value = (sai_attribute_value_t){.ipaddr = udip1}},
        {.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_OVERLAY_DMAC, .value = (sai_attribute_value_t){.mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x56}}},
        {.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_USE_DST_VNET_VNI, .value = (sai_attribute_value_t){.booldata = false}},
        {.id = SAI_OUTBOUND_CA_TO_PA_ENTRY_ATTR_COUNTER_ID, .value = (sai_attribute_value_t){.oid = counter1}}
    };

    const sai_attribute_t *attr_list[entries_count] = {
        attrs0,
        attrs1,
    };

    uint32_t attr_count[entries_count] = {entry_attrs_count, entry_attrs_count};
    sai_status_t statuses[entries_count] = {};

    sai_outbound_ca_to_pa_entry_t entries[entries_count] = {
        { .switch_id = switchid, .dst_vnet_id = vnet0, .dip = dip0},
        { .switch_id = switchid, .dst_vnet_id = vnet1, .dip = dip1},
    };

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkCreate(entries_count, entries, attr_count, attr_list, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    EXPECT_EQ(SAI_STATUS_SUCCESS, m.bulkRemove(entries_count, entries, SAI_BULK_OP_ERROR_MODE_STOP_ON_ERROR, statuses));
    for (uint32_t i = 0; i < entries_count; i++) {
        EXPECT_EQ(SAI_STATUS_SUCCESS, statuses[i]);
    }

    remove_vnet(m, vnet0);
    remove_vnet(m, vnet1);
    remove_counter(m, counter0);
    remove_counter(m, counter1);
}
