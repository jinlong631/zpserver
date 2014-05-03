#include "st_cross_svr_node.h"
#include "../cluster/zp_clusterterm.h"
#include "st_client_table.h"
#include "st_message.h"
namespace SmartLink{

	st_cross_svr_node::st_cross_svr_node(ZP_Cluster::zp_ClusterTerm * pTerm, QObject * psock,QObject *parent)
		:ZP_Cluster::zp_ClusterNode(pTerm,psock,parent)
	{
		m_currStMegSize = 0;
		m_destin_uuid = 0xffffffff;
	}
	int st_cross_svr_node::st_bytesLeft()
	{
		return m_st_Header.messageLen + sizeof(STCROSSSVR_MSG::tag_msgHearder) - m_currStMegSize ;
	}

	bool st_cross_svr_node::deal_user_data(const QByteArray &array)
	{
		const char * pData = array.constData();
		int nBlockSize = array.size();
		int nOffset = 0;
		while (nOffset < nBlockSize )
		{
			while (m_currStMegSize < sizeof(STCROSSSVR_MSG::tag_msgHearder) && nOffset< nBlockSize)
			{
				m_currStBlock.push_back(pData[nOffset++]);
				++m_currStMegSize;
			}
			if (m_currStMegSize < sizeof(STCROSSSVR_MSG::tag_msgHearder))
				return true;
			if (m_currStMegSize == sizeof(STCROSSSVR_MSG::tag_msgHearder))
			{
				memcpy (&m_st_Header,m_currStBlock.constData(),sizeof(STCROSSSVR_MSG::tag_msgHearder));
				if (m_st_Header.Mark != 0x4567)
				{
					m_currStMegSize = 0;
					m_currStBlock.clear();
					return true;
				}
			}
			while (nOffset<nBlockSize && m_currStMegSize < m_st_Header.messageLen + sizeof(STCROSSSVR_MSG::tag_msgHearder) )
			{
				m_currStBlock.push_back(pData[nOffset++]);
				++m_currStMegSize;
			}
			bool needDel = deal_msg();
			if (st_bytesLeft()==0 || needDel==true)
			{
				m_currStMegSize = 0;
				m_currStBlock.clear();
				m_destin_uuid = 0xffffffff;
			}
		}
		return ZP_Cluster::zp_ClusterNode::deal_user_data(array);
	}
	bool  st_cross_svr_node::deal_msg()
	{
		bool delCurrBlock = false;
		switch (m_st_Header.mesageType)
		{
		case 0x01: //client node log in
		{
			if (st_bytesLeft()>0)
				return delCurrBlock;
			STCROSSSVR_MSG * pMsg = (STCROSSSVR_MSG *) m_currStBlock.constData();
			int nUUIDs = pMsg->header.messageLen / sizeof(quint32);
			this->m_pClientTable->cross_svr_add_uuids(this->termName(),pMsg->payload.uuids,nUUIDs);
		}
			break;
		case 0x02: //client node exit
		{
			if (st_bytesLeft()>0)
				return delCurrBlock;
			STCROSSSVR_MSG * pMsg = (STCROSSSVR_MSG *) m_currStBlock.constData();
			int nUUIDs = pMsg->header.messageLen / sizeof(quint32);
			this->m_pClientTable->cross_svr_del_uuids(this->termName(),pMsg->payload.uuids,nUUIDs);
		}
			break;
		case 0x03: // data transfer
		{
			if (m_destin_uuid == 0xffffffff)
			{
				if (m_currStMegSize >= sizeof(STCROSSSVR_MSG::tag_msgHearder)+ sizeof(SMARTLINK_MSG)-1)
				{
					STCROSSSVR_MSG * pMsg = (STCROSSSVR_MSG *) m_currStBlock.constData();
					SMARTLINK_MSG * pSmMsg = (SMARTLINK_MSG *) pMsg->payload.data;
					m_destin_uuid = pSmMsg->destin_id;
				}
			}
			if (m_destin_uuid == 0xffffffff)
				return false;
			//Send to Client
//			//Transfer
//			if (false==m_pClientTable->SendToNode(this->m_destin_uuid , m_currStBlock))
//			{

//			}

		}
			delCurrBlock = true;
			break;
		default:
			break;
		}

		return delCurrBlock;

	}

	void st_cross_svr_node::setClientTable(st_client_table * table)
	{
		this->m_pClientTable = table;
	}

}