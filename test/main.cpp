
#include "lvdb/lvdb.h"
#include "lvdb/sync.h"
#include "toolkits/util.h"
#include "gtest/gtest.h"

TEST(LVDBTest, DBApi)
{
	lv::Options opt;
	lv::LVDB *db = lv::LVDB::open(opt);
	std::string v;
	//hash
	lv::Bytes hn = lv::Bytes("test");
	lv::Bytes hk0 = lv::Bytes("0");
	lv::Bytes hk1 = lv::Bytes("1");
	lv::Bytes hk2 = lv::Bytes("2");
	lv::Bytes hk3 = lv::Bytes("3");
	lv::Bytes hk4 = lv::Bytes("4");
	lv::Bytes hk5 = lv::Bytes("5");
	db->hset(hn, hk0, lv::Bytes("1000"));
	db->hset(hn, hk1, lv::Bytes("1001"));
	db->hset(hn, hk2, lv::Bytes("1002"));
	db->hset(hn, hk3, lv::Bytes("1003"));
	db->hset(hn, hk4, lv::Bytes("1004"));
	db->hset(hn, hk5, lv::Bytes("1005"));
 	db->set(hk5, lv::Bytes("1005"));
	db->hset(hn, hk5, lv::Bytes("10051"));
	EXPECT_EQ(6, db->hsize(hn));
	EXPECT_GE(0, db->hget(hn, hn, &v));
	EXPECT_LT(0, db->hget(hn, hk2, &v));

	printf("scan from %s to %s\n", hk1.String().c_str(), hk3.String().c_str());
	lv::HIterator* hs = db->hscan(hn, hk1, hk3, 5);
	while (hs->next())
	{
		printf("%s = %s\n", hs->key.c_str(), hs->val.c_str());
	}
	hs->release();
	printf("rscan from %s to %s\n", hk3.String().c_str(), hk1.String().c_str());
	lv::HIterator* rhs = db->hrscan(hn, hk3, hk1, 5);
	while (rhs->next())
	{
		printf("%s = %s\n", rhs->key.c_str(), rhs->val.c_str());
	}
	rhs->release();

	lv::Null_Sync_Processor null_sync;
	lv::Sync *sync = new lv::Sync("test", db, &null_sync);
	sync->create();
	sync->start();

	getchar();
	db->release();
}

int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}