/************************************
 * file enc : utf8
 * author   : wuyanyi09@gmail.com
************************************/
#include "Trie.h"

namespace CppJieba
{
    Trie::iterator Trie::begin()
    {
        return _nodeInfoVec.begin();
    }

    Trie::iterator Trie::end()
    {
        return _nodeInfoVec.end();
    }

    Trie::Trie()
    {

		_root = NULL;
		_totalCount = 0;
		_minWeight = numeric_limits<double>::max();
		_initFlag = false;
    }
    
    Trie::~Trie()
    {
        dispose();
    }
	
	bool Trie::_getInitFlag()
	{
		return _initFlag;
	}
	void Trie::_setInitFlag(bool on)
	{
		_initFlag = on;
	}

	bool Trie::init()
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

	bool Trie::loadDict(const string& filePath)
	{
		if(!_getInitFlag())
		{
			LogError("not initted.");
			return false;
		}

		if(!checkFileExist(filePath.c_str()))
		{
			LogError(string_format("cann't find fiel[%s].",filePath.c_str()));
			return false;
		}
		bool res = false;
		res = _buildTree(filePath);
		if(!res)
		{
			LogError("_buildTree failed.");
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

    bool Trie::_buildTree(const string& filePath)
    {
		
        ifstream ifile(filePath.c_str());
        string line;
        vector<string> vecBuf;
        while(getline(ifile, line))
        {
            vecBuf.clear();
            splitStr(line, vecBuf, " ");
            if(3 != vecBuf.size())
            {
                LogError(string_format("line[%s] illegal.", line.c_str()));
                return false;
            }
            string chWord = vecBuf[0];
            uint count = atoi(vecBuf[1].c_str());
            const string& tag = vecBuf[2];

			//insert node
			TrieNodeInfo nodeInfo;
			nodeInfo.word = chWord;
			nodeInfo.wLen = 0;
			nodeInfo.count = count;
			nodeInfo.tag = tag;

			bool flag = insert(nodeInfo);
			if(!flag)
			{
				LogError("insert node failed!");
				return false;
			}
			

        }
        return true;
    }

    bool Trie::dispose()
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

	const TrieNodeInfo* Trie::findPrefix(const string& str)
	{
		if(!_getInitFlag())
		{
			LogFatal("trie not initted!");
			return NULL;
		}
		VUINT16 unicode;
		
		bool retFlag = TransCode::strToVec(str, unicode);
		if(retFlag)
		{
			LogError("TransCode::strToVec failed.");
			return NULL;
		}

		//find
		TrieNode* p = _root;
		TrieNodeInfo * res = NULL;
		for(uint i = 0; i < unicode.size(); i++)
		{
			uint16_t chUni = unicode[i];
			if(p->isLeaf)
			{
				uint pos = p->nodeInfoVecPos;
				if(pos < _nodeInfoVec.size())
				{
					res = &(_nodeInfoVec[pos]);
				}
				else
				{
					LogFatal("node's nodeInfoVecPos is out of _nodeInfoVec's range");
					return NULL;
				}
				
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

	const TrieNodeInfo* Trie::find(const string& str)
	{
		VUINT16 unicode;
		bool retFlag = TransCode::strToVec(str, unicode);
		if(!retFlag)
		{
			return NULL;
		}
		return find(unicode);
	}

	const TrieNodeInfo* Trie::find(const VUINT16& unicode)
	{
		if(unicode.empty())
		{
			return NULL;
		}
		return find(unicode.begin(), unicode.end());
	}

	const TrieNodeInfo* Trie::find(VUINT16_CONST_ITER begin, VUINT16_CONST_ITER end)
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
		for(VUINT16_CONST_ITER it = begin; it != end; it++)
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

	double Trie::getWeight(const string& str)
	{

		VUINT16 unicode;
		TransCode::strToVec(str, unicode);
		return getWeight(unicode);
	}

	double Trie::getWeight(const VUINT16& unicode)
	{
		if(unicode.empty())
		{
			return getMinWeight();
		}
		const TrieNodeInfo * p = find(unicode);
		if(NULL != p)
		{
			return p->weight;
		}
		else
		{
			return getMinWeight();
		}
		
	}

	double Trie::getWeight(VUINT16_CONST_ITER begin, VUINT16_CONST_ITER end)
	{
		const TrieNodeInfo * p = find(begin, end);
		if(NULL != p)
		{
			return p->weight;
		}
		else
		{
			return getMinWeight();
		}
	}

	double Trie::getMinWeight()
	{
		return _minWeight;
	}

	int64_t Trie::getTotalCount()
	{
		return _totalCount;
	}

    bool Trie::_deleteNode(TrieNode* node)
    {
        for(TrieNodeMap::iterator it = node->hmap.begin(); it != node->hmap.end(); it++)
        {
            TrieNode* next = it->second;
            _deleteNode(next);
        }
        
        delete node;
        return true;
    }


	bool Trie::insert(const TrieNodeInfo& nodeInfo)
	{
		if(!_getInitFlag())
		{
			LogFatal("not initted!");
			return false;
		}

		const string& word = nodeInfo.word;
		
		VUINT16 unicode;
		bool retFlag = TransCode::strToVec(word, unicode);
		if(!retFlag)
		{
			LogError("TransCode::strToVec error.");
			return false;
		}
		
        TrieNode* p = _root;
        for(uint i = 0; i < unicode.size(); i++)
        {
			uint16_t cu = unicode[i];
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

	bool Trie::_countWeight()
	{
		if(_nodeInfoVec.empty() || 0 != _totalCount)
		{
			LogError("_nodeInfoVec is empty or _totalCount has been counted already.");
			return false;
		}
		
		//count total freq
		for(size_t i = 0; i < _nodeInfoVec.size(); i++)
		{
			_totalCount += _nodeInfoVec[i].count;
		}

		if(0 == _totalCount)
		{
			LogError("_totalCount == 0 .");
			return false;
		}
		
		//normalize
		for(uint i = 0; i < _nodeInfoVec.size(); i++)
		{
			TrieNodeInfo& nodeInfo = _nodeInfoVec[i];
			if(0 == nodeInfo.count)
			{
				LogFatal("nodeInfo.count == 0!");
				return false;
			}
			nodeInfo.weight = log(double(nodeInfo.count)/double(_totalCount));
			if(_minWeight > nodeInfo.weight)
			{
				_minWeight = nodeInfo.weight;
			}
		}
		
		return true;
	}
}

#ifdef TRIE_UT
using namespace CppJieba;
int main()
{
    Trie trie;
    trie.init();
	trie.loadDict("../dicts/segdict.gbk.v2.1");
	//trie.loadDict("tmp");
	cout<<trie.getMinWeight()<<endl;
	cout<<trie.getTotalCount()<<endl;
    trie.dispose();
    return 0;
}
#endif
