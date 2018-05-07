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
        (  1, uint256("0x997dba4502f207bac4c1ea25eb503fa899bc52ca5ca518de1e63e0b61631d8c6"))
        (  2, uint256("0xccf7e064c44af1159d759a7b264133fa9eb397a6032d2d606b5e533bf8b0d9b4"))
        (  3, uint256("0xebcc738bfbb4a06e6a160af3358c2add7e79d6642274ac6e0f8ce9f482aa0689"))
        (  4, uint256("0xecabd6dbab7635d9a6b858d40d642d333f448be5f3a1365d8e11b92f9352911d"))
        (  5, uint256("0x25f3e2f4ee60bbb6cc718c9ead3bb13911695e0672a318f11366d1e9adab3a65"))
        (  6, uint256("0xe80aab5c0e58d4f82b3c45673c95abdca0bf4c7e0f910feaea2e24b0d22c8790"))
        (  7, uint256("0x6b4aaf10bac84ae96dce72b93f244429798b3176269a6510b476d8d9d804b0f9"))
        (  8, uint256("0x6997a933515c037d5ada50885808dca2ae3775f262a08266b22f8ad9e664fe39"))
        (  9, uint256("0x6b5f2640960f7505bab356976f84b3e7c3ab3ff0d05687bd9effa7f43b44f755"))
        (  10, uint256("0x016abe44b10765e4bf7ea260e371c076581759154d18e87dd8fa6af055e85ec0"))
        (  11, uint256("0x94b3e5cbeeced3c003375a091e3cf5a510550eaf1dbd81aebaa91a4a305718bd"))
        (  12, uint256("0x116067e796d572dd8e34b67e2aa6e687eb0b30c59481c354ce0ccff1c5ba13c6"))
        (  13, uint256("0x763be5b3548a0bb3d7d02e871f5a90a37de6875700024da83a7d09514d2876a5"))
        ;
    static const CCheckpointData data = {
        &mapCheckpoints,
        1525686263, // * UNIX timestamp of last checkpoint block
        14,    // * total number of transactions between genesis and last checkpoint
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
