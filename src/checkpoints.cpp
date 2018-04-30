// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double fSigcheckVerificationFactor = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64 nTimeLastCheckpoint;
        int64 nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (  0, uint256("0x59ffe119282b34f0670d421b82b60ec74b2c383444e1b8b0b12ab037934c1cdf"))
        (  1, uint256("0x8a0254f5f72126c074380c592adc6636fc02dbb038b5ad1265a92226078a9d09"))
        (  2, uint256("0xe4755f75dd2ce5cc7972c2c84a979dd1898788dbc10943149772bdc8c3e5bea8"))
        (  3, uint256("0xbb8b9d95814ded1b7513fd802c3f90efc22ba31d35fbb7f923f643cc92b029af"))
        (  4, uint256("0xb84ded6303dc510ac2d75c221a12c75a4059ad6c79faf0409497dd29a462606d"))
        (  5, uint256("0x6e7d2ba749e87926121241e25026a4cb4dd737736e9a66d40bd3698fba503a17"))
        (  6, uint256("0xc1e17019c51d59602d67ececd9f01f0d0b6292d162c1ea9705b70f70a45ff2bc"))
        (  7, uint256("0x9269230a3c4b1d8dfdbf10cdc69267ac299813f44cdb43ece8437c35f33e8e3a"))
        (  8, uint256("0xc71ff7b44a77ac275fb51b46f79a8dd2f9f37401734f47105e39fedfb033ab14"))
        (  9, uint256("0x96a2fb229a458a51a06d7014c3fd0bc6c39b79f42581ee0141d766617a41b2a6"))
        (  10, uint256("0xdddff615c6716c9a42f23673d1a1f6e4986861e5f3a03d3d05506153d7a90069"))
        (  11, uint256("0x2431c4509fff4673b6db3d1ceeeeeb99733d7dca24b0657c6b10b52c243b5224"))
        (  12, uint256("0x9321d115318f5c943446b3159338c36b644c5628715aefabaf697bafdffd5167"))
        (  13, uint256("0xa8d1cbe2c33be8eb590cc84d148cfdf4aed02321d6244f773b0d190f49204ae8"))
        (  14, uint256("0x6ed91b697eaeeb9b6893776e5cc400dff34de5c9ac5c2ecb7ced1035492d49d4"))
        (  15, uint256("0xdc81a2b3508e218be1c3684a8083cca462a868ae6139cb0bede68ca21ffa5a4b"))
        ;
    static const CCheckpointData data = {
        &mapCheckpoints,
        1523971019, // * UNIX timestamp of last checkpoint block
        15,    // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        10000000     // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        (   0, uint256("0x"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        //1521262923,
        //15,
        //10000000
    };

    const CCheckpointData &Checkpoints() {
        if (fTestNet)
            return dataTestnet;
        else
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!GetBoolArg("-checkpoints", true))
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex) {
        if (pindex==NULL)
            return 0.0;

        int64 nNow = time(NULL);

        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (!GetBoolArg("-checkpoints", true))
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!GetBoolArg("-checkpoints", true))
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}