// Copyright (c) 2013-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockrelay/thinblock.h"
#include "blockrelay/blockrelay_common.h"
#include "net.h"
#include "test/test_bitcoin.h"
#include <assert.h>
#include <boost/range/irange.hpp>
#include <boost/test/unit_test.hpp>
#include <limits>
#include <string>
#include <vector>

using namespace std;

class TestTBD : public CThinBlockData
{
protected:
    virtual int64_t getTimeForStats() { return times[min(times_idx++, times.size() - 1)]; }
    vector<int64_t> times;
    size_t times_idx;

public:
    TestTBD(const vector<int64_t> &_times)
    {
        assert(_times.size() > 0);
        times = _times;
        times_idx = 0;
    }
    void resetTimeIdx() { times_idx = 0; }
};


BOOST_FIXTURE_TEST_SUITE(thinblock_data_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_thinblock_byte_tracking)
{
    CThinBlockData thindata;

    /**
     * Do calcuations for single peer building a thinblock
     */

    CAddress addr1(ipaddress(0xa0b0c001, 10000));
    CNode dummyNode1(INVALID_SOCKET, addr1, "", true);

    CXThinBlock xthin;
    std::shared_ptr<CBlockThinRelay> pblock = std::make_shared<CBlockThinRelay>(CBlockThinRelay());
    pblock->xthinblock = std::make_shared<CXThinBlock>(xthin);

    thinrelay.ResetTotalBlockBytes();
    BOOST_CHECK(0 == thinrelay.GetTotalBlockBytes());
    BOOST_CHECK(0 == pblock->nCurrentBlockSize);

    thinrelay.AddTotalBlockBytes(0, pblock);
    BOOST_CHECK(0 == thinrelay.GetTotalBlockBytes());
    BOOST_CHECK(0 == pblock->nCurrentBlockSize);

    thinrelay.AddTotalBlockBytes(1000, pblock);
    BOOST_CHECK(1000 == thinrelay.GetTotalBlockBytes());
    BOOST_CHECK(1000 == pblock->nCurrentBlockSize);

    thinrelay.AddTotalBlockBytes(449932, pblock);
    BOOST_CHECK(450932 == thinrelay.GetTotalBlockBytes());
    BOOST_CHECK(450932 == pblock->nCurrentBlockSize);

    thinrelay.DeleteTotalBlockBytes(0);
    BOOST_CHECK(450932 == thinrelay.GetTotalBlockBytes());
    BOOST_CHECK(450932 == pblock->nCurrentBlockSize);

    thinrelay.DeleteTotalBlockBytes(1);
    BOOST_CHECK(450931 == thinrelay.GetTotalBlockBytes());

    thinrelay.DeleteTotalBlockBytes(13939);
    BOOST_CHECK(436992 == thinrelay.GetTotalBlockBytes());

    // Try to delete more bytes than we already have tracked.  This should not be possible...we don't allow this
    // to happen in the event that we get an incorrect or invalid value returned for the dynamic memory usage of
    // a transaction.  This could then be used in a theoretical attack by resetting total byte usage to zero while
    // continuing to build more thinblocks.
    thinrelay.DeleteTotalBlockBytes(436993);
    BOOST_CHECK_MESSAGE(
        436992 == thinrelay.GetTotalBlockBytes(), "nThinBlockBytes is " << thinrelay.GetTotalBlockBytes());


    /**
     * Add a second peer and do more calcuations for building a second thinblock
     */

    CAddress addr2(ipaddress(0xa0b0c002, 10000));
    CNode dummyNode2(INVALID_SOCKET, addr2, "", true);
    pblock->SetNull();

    thinrelay.AddTotalBlockBytes(1000, pblock);
    BOOST_CHECK(437992 == thinrelay.GetTotalBlockBytes());
    BOOST_CHECK(1000 == pblock->nCurrentBlockSize);

    thinrelay.DeleteTotalBlockBytes(0);
    BOOST_CHECK(437992 == thinrelay.GetTotalBlockBytes());

    thinrelay.DeleteTotalBlockBytes(1);
    BOOST_CHECK(437991 == thinrelay.GetTotalBlockBytes());

    thinrelay.DeleteTotalBlockBytes(999);
    BOOST_CHECK(436992 == thinrelay.GetTotalBlockBytes());

    // now finally reset everything
    thinrelay.ResetTotalBlockBytes();
    BOOST_CHECK_MESSAGE(0 == thinrelay.GetTotalBlockBytes(), "nThinBlockBytes is " << thinrelay.GetTotalBlockBytes());
}

BOOST_AUTO_TEST_CASE(test_thinblockdata_stats1)
{
    vector<int64_t> times1(1000); // minutes

    for (uint32_t i = 0; i < times1.size(); i++)
    {
        times1[i] = 1000 * 60 * i;
    }

    {
        TestTBD tbd(times1);
        // exercise summary methods on empty arrays to make sure they don't fail
        // in weird ways
        tbd.ToString();
        tbd.InBoundPercentToString();
        tbd.OutBoundPercentToString();
        tbd.InBoundBloomFiltersToString();
        tbd.OutBoundBloomFiltersToString();
        tbd.ResponseTimeToString();
        tbd.ValidationTimeToString();
        tbd.ReRequestedTxToString();
        tbd.MempoolLimiterBytesSavedToString();
    }

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateInBound(i, 3 * i);

        string res = tbd.InBoundPercentToString();

        BOOST_CHECK_MESSAGE(res.find("66.7%") != string::npos, "InBoundPercentToString() is " << res);
    }

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateOutBound(i, 3 * i);

        string res = tbd.OutBoundPercentToString();

        BOOST_CHECK_MESSAGE(res.find("66.7%") != string::npos, "OutBoundPercentToString() is " << res);
    }

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateInBoundBloomFilter(1000 * i);

        string res = tbd.InBoundBloomFiltersToString();

        BOOST_CHECK_MESSAGE(res.find("49.50KB") != string::npos, "InBoundBloomFiltersToString() is " << res);
    }

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateOutBoundBloomFilter(1000 * i);

        string res = tbd.OutBoundBloomFiltersToString();
        BOOST_CHECK_MESSAGE(res.find("49.50KB") != string::npos, "OutBoundBloomFiltersToString() is " << res);
    }
    // FIXME: check others somehow that depend on chain sync state

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateInBoundReRequestedTx(1000 * i);

        string res = tbd.ReRequestedTxToString();
        BOOST_CHECK_MESSAGE(res.find(":100") != string::npos, "ReRequestedTxToString() is " << res);
    }

    {
        TestTBD tbd(times1);

        for (int64_t i : boost::irange(0, 100))
            tbd.UpdateMempoolLimiterBytesSaved(1000 * i);

        string res = tbd.MempoolLimiterBytesSavedToString();
        BOOST_CHECK_MESSAGE(res.find("4.95MB") != string::npos, "MempoolLimiterBytesSavedToString() is " << res);
    }
}

BOOST_AUTO_TEST_SUITE_END()
