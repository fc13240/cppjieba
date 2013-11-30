/************************************
 * file enc : ASCII
 * author   : wuyanyi09@gmail.com
 ************************************/
#ifndef CPPJIEBA_TRIE_H
#define CPPJIEBA_TRIE_H

#include <iostream>
#include <fstream>
#include <map>
#include <cstring>
#include <stdint.h>
#include <cmath>
#include <limits>
#include "Limonp/str_functs.hpp"
#include "Limonp/logger.hpp"
#include "TransCode.hpp"
#include "globals.h"
#include "structs.h"


namespace CppJieba
{
    using namespace Limonp;
    struct TrieNode
    {
        TrieNodeMap hmap;
        bool isLeaf;
        uint nodeInfoVecPos;
        TrieNode()
        {
            isLeaf = false;
            nodeInfoVecPos = 0;
        }
    };

    class Trie
    {

        private:
            TrieNode* _root;
            vector<TrieNodeInfo> _nodeInfoVec;

            bool _initFlag;
            int64_t _freqSum;
            double _minLogFreq;

        public:
            Trie()
            {
                _root = NULL;
                _freqSum = 0;
                _minLogFreq = MAX_DOUBLE;
                _initFlag = false;
            }
            ~Trie()
            {
                dispose();
            }
            bool init()
            {
                if(_getInitFlag())
                {
                    LogError("already initted!");
                    return false;
                }

                try
                {
                    _root = new TrieNode;
                }
                catch(const bad_alloc& e)
                {
                    return false;
                }
                if(NULL == _root)
                {
                    return false;
                }
                _setInitFlag(true);
                return true;
            }
            bool dispose()
            {
                if(!_getInitFlag())
                {
                    return false;
                }
                bool ret = _deleteNode(_root);
                if(!ret)
                {
                    LogFatal("_deleteNode failed!");
                    return false;
                }
                _root = NULL;
                _nodeInfoVec.clear();

                _setInitFlag(false);
                return ret;
            }
            bool loadDict(const char * const filePath)
            {
                if(!_getInitFlag())
                {
                    LogError("not initted.");
                    return false;
                }

                if(!checkFileExist(filePath))
                {
                    LogError("cann't find fiel[%s].",filePath);
                    return false;
                }
                bool res = false;
                res = _trieInsert(filePath);
                if(!res)
                {
                    LogError("_trieInsert failed.");
                    return false;
                }
                res = _countWeight();
                if(!res)
                {
                    LogError("_countWeight failed.");
                    return false;
                }
                return true;
            }

        private:
            void _setInitFlag(bool on){_initFlag = on;};
            bool _getInitFlag()const{return _initFlag;};

        public:
            const TrieNodeInfo* find(const string& str)const
            {
                Unicode uintVec;
                if(!TransCode::decode(str, uintVec))
                {
                    return NULL;
                }
                return find(uintVec);
            }
            const TrieNodeInfo* find(const Unicode& uintVec)const
    {
        if(uintVec.empty())
        {
            return NULL;
        }
        return find(uintVec.begin(), uintVec.end());
    }
            const TrieNodeInfo* find(Unicode::const_iterator begin, Unicode::const_iterator end)const
    {
        
        if(!_getInitFlag())
        {
            LogFatal("trie not initted!");
            return NULL;
        }
        if(begin >= end)
        {
            return NULL;
        }
        TrieNode* p = _root;
        for(Unicode::const_iterator it = begin; it != end; it++)
        {
            uint16_t chUni = *it;
            if(p->hmap.find(chUni) == p-> hmap.end())
            {
                return NULL;
            }
            else
            {
                p = p->hmap[chUni];
            }
        }
        if(p->isLeaf)
        {
            uint pos = p->nodeInfoVecPos;
            if(pos < _nodeInfoVec.size())
            {
                return &(_nodeInfoVec[pos]);
            }
            else
            {
                LogFatal("node's nodeInfoVecPos is out of _nodeInfoVec's range");
                return NULL;
            }
        }
        return NULL;
    }
            bool find(const Unicode& unico, vector<pair<uint, const TrieNodeInfo*> >& res)const
	{
        if(!_getInitFlag())
        {
            LogFatal("trie not initted!");
            return false;
        }
        TrieNode* p = _root;
        for(uint i = 0; i < unico.size(); i++)
        {
            if(p->hmap.find(unico[i]) == p-> hmap.end())
            {
				break;
            }
			p = p->hmap[unico[i]];
			if(p->isLeaf)
			{
				uint pos = p->nodeInfoVecPos;
				if(pos < _nodeInfoVec.size())
				{
					res.push_back(make_pair(i, &_nodeInfoVec[pos]));
				}
				else
				{
					LogFatal("node's nodeInfoVecPos is out of _nodeInfoVec's range");
					return false;
				}
			}
        }
		return !res.empty();
	}

            const TrieNodeInfo* findPrefix(const string& str)const
            {
                if(!_getInitFlag())
                {
                    LogFatal("trie not initted!");
                    return NULL;
                }
                Unicode uintVec;

                if(!TransCode::decode(str, uintVec))
                {
                    LogError("TransCode::decode failed.");
                    return NULL;
                }

                //find
                TrieNode* p = _root;
                uint pos = 0;
                uint16_t chUni = 0;
                const TrieNodeInfo * res = NULL;
                for(uint i = 0; i < uintVec.size(); i++)
                {
                    chUni = uintVec[i];
                    if(p->isLeaf)
                    {
                        pos = p->nodeInfoVecPos;
                        if(pos >= _nodeInfoVec.size())
                        {
                            LogFatal("node's nodeInfoVecPos is out of _nodeInfoVec's range");
                            return NULL;
                        }
                        res = &(_nodeInfoVec[pos]);

                    }
                    if(p->hmap.find(chUni) == p->hmap.end())
                    {
                        break;
                    }
                    else
                    {
                        p = p->hmap[chUni];
                    }
                }
                return res;
            }

        public:
            double getMinLogFreq()const{return _minLogFreq;};

            bool insert(const TrieNodeInfo& nodeInfo)
            {
                if(!_getInitFlag())
                {
                    LogFatal("not initted!");
                    return false;
                }


                const Unicode& uintVec = nodeInfo.word;
                TrieNode* p = _root;
                for(uint i = 0; i < uintVec.size(); i++)
                {
                    uint16_t cu = uintVec[i];
                    if(NULL == p)
                    {
                        return false;
                    }
                    if(p->hmap.end() == p->hmap.find(cu))
                    {
                        TrieNode * next = NULL;
                        try
                        {
                            next = new TrieNode;
                        }
                        catch(const bad_alloc& e)
                        {
                            return false;
                        }
                        p->hmap[cu] = next;
                        p = next;
                    }
                    else
                    {
                        p = p->hmap[cu];
                    }
                }
                if(NULL == p)
                {
                    return false;
                }
                if(p->isLeaf)
                {
                    LogError("this node already inserted");
                    return false;
                }

                p->isLeaf = true;
                _nodeInfoVec.push_back(nodeInfo);
                p->nodeInfoVecPos = _nodeInfoVec.size() - 1;

                return true;
            }

        private:
            bool _trieInsert(const char * const filePath)
            {
                ifstream ifile(filePath);
                string line;
                vector<string> vecBuf;

                TrieNodeInfo nodeInfo;
                while(getline(ifile, line))
                {
                    vecBuf.clear();
                    splitStr(line, vecBuf, " ");
                    if(3 < vecBuf.size())
                    {
                        LogError("line[%s] illegal.", line.c_str());
                        return false;
                    }
                    if(!TransCode::decode(vecBuf[0], nodeInfo.word))
                    {
                        return false;
                    }
                    nodeInfo.freq = atoi(vecBuf[1].c_str());
                    if(3 == vecBuf.size())
                    {
                        nodeInfo.tag = vecBuf[2];
                    }

                    //insert node
                    if(!insert(nodeInfo))
                    {
                        LogError("insert node failed!");
                    }
                }
                return true;
            }
            bool _countWeight()
            {
                if(_nodeInfoVec.empty() || 0 != _freqSum)
                {
                    LogError("_nodeInfoVec is empty or _freqSum has been counted already.");
                    return false;
                }

                //freq total freq
                for(size_t i = 0; i < _nodeInfoVec.size(); i++)
                {
                    _freqSum += _nodeInfoVec[i].freq;
                }

                if(0 == _freqSum)
                {
                    LogError("_freqSum == 0 .");
                    return false;
                }

                //normalize
                for(uint i = 0; i < _nodeInfoVec.size(); i++)
                {
                    TrieNodeInfo& nodeInfo = _nodeInfoVec[i];
                    if(0 == nodeInfo.freq)
                    {
                        LogFatal("nodeInfo.freq == 0!");
                        return false;
                    }
                    nodeInfo.logFreq = log(double(nodeInfo.freq)/double(_freqSum));
                    if(_minLogFreq > nodeInfo.logFreq)
                    {
                        _minLogFreq = nodeInfo.logFreq;
                    }
                }

                return true;
            }

            bool _deleteNode(TrieNode* node)
            {
                for(TrieNodeMap::iterator it = node->hmap.begin(); it != node->hmap.end(); it++)
                {
                    TrieNode* next = it->second;
                    _deleteNode(next);
                }

                delete node;
                return true;
            }

    };
}

#endif