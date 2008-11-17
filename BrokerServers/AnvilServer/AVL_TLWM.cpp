#include "stdAfx.h"
#include "AVL_TLWM.h"
#include "Messages.h"

	std::vector <CString>client;
	std::vector <time_t>heart;
	typedef std::vector <CString> mystocklist;
	std::vector < mystocklist > stocks;
	std::vector <Observer*> subs; // this saves our subscriptions with the hammer server
	std::vector <CString> subsym;

namespace TradeLibFast
{
	// private structures
	std::vector<CString> accounts;
	std::vector<Order*> ordercache;
	




	AVL_TLWM::AVL_TLWM(void)
	{
		void* iterator = B_CreateAccountIterator();
		B_StartIteration(iterator);
		Observable* acct;
		while (acct = B_GetNextAccount(iterator)) // loop through every available account
		{
			acct->Add(this); // add this object to account as an observer
			accounts.push_back(acct); // save the account
		}
		B_DestroyIterator(iterator);
	}

	AVL_TLWM::~AVL_TLWM(void)
	{
		void* iterator = B_CreateAccountIterator();
		B_StartIteration(iterator);
		Observable* acct;
		while (acct = B_GetNextAccount(iterator)) // loop through every available account
		{
			acct->Remove(this); // add this object to account as an observer
		}
		B_DestroyIterator(iterator);
		accounts.clear();
		ordercache.clear();
	}

	int AVL_TLWM::BrokerName(void)
	{
		return Assent;
	}

	int AVL_TLWM::RegisterStocks(CString clientname)
	{
/*		int cid = FindClient(clientname);
		vector<CString> my = this->stocks[cid]; // get this clients stocks
		for (size_t i = 0; i<my.size();i++) // subscribe to stocks
		{
			if (hasHammerSub(my[i])) continue; // if we've already subscribed once, skip to next stock
			AVLStock* stk = new AVLStock(my[i],this); // create new stock instance
			stk->Load();
			this->stocksubs.push_back(stk);
		}
		*/
		return OK;
	}

	bool AVL_TLWM::hasHammerSub(CString symbol)
	{
		for (size_t i = 0; i<stocksubs.size(); i++) 
			if (symbol.CompareNoCase(stocksubs[i]->GetSymbol().c_str())==0) 
				return true;
		return false;
	}

	int AVL_TLWM::SendOrder(TLOrder order)
	{
		Observable* m_account;
		m_account = B_GetCurrentAccount();
		
		const StockBase* Stock = B_GetStockHandle(order.symbol);

		//convert the arguments
		Order* orderSent;
		char side = order.side ? 'B' : 'S';

		const Money pricem = Money((int)(order.price*1024));
		const Money stopm = Money((int)(order.stop*1024));
		unsigned int tif = TIF_DAY;

		// send the order (next line is from SendOrderDlg.cpp)
		unsigned int error = B_SendOrder(Stock,
				side,
				order.exchange,
				order.size,
				OVM_VISIBLE, //visability mode
				order.size, //visable size
				pricem,//const Money& price,0 for Market
				&stopm,//const Money* stopPrice,
				NULL,//const Money* discrtetionaryPrice,
				tif,
				false,//bool proactive,
				true,//bool principalOrAgency, //principal - true, agency - false
				SUMO_ALG_UNKNOWN,//char superMontageAlgorithm,
				OS_RESIZE,
				//false,//bool delayShortTillUptick,
				DE_DEFAULT, //destination exchange
				&orderSent,
				m_account,
				0,
				false,
				101, order.comment);	

		// cleanup all the stuff we created
		Stock = NULL;
		orderSent = NULL;
		m_account = NULL;
		return error;
	}

	int AccountId(CString acct) { for (size_t i = 0; i<accounts.size(); i++) if (accounts[i]==acct) return i; return -1; }


	bool hasOrder(unsigned int  TLid)
	{
		return (TLid>=0) && (TLid<ordercache.size());
	}

	int AVL_TLWM::cacheOrder(Order* o)
	{
		for (unsigned int i = 0; i<ordercache.size(); i++)
			if (ordercache[i]==o) 
				return i; // found order so we return it's index
		ordercache.push_back(o);
		return ordercache.size()-1; // didn't find order so we added it and returned index
	}

	void AVL_TLWM::Process(const Message* message, Observable* from, const Message* additionalInfo)
	{
		switch(message->GetType())
		{
			case M_POOL_EXECUTION:
			if(additionalInfo != NULL && additionalInfo->GetType() == M_AI_EXECUTION)
			{
				 MsgPoolExecution* msg = (MsgPoolExecution*)message;//to get the structure, just cast Message* to  MsgPoolExecution* (not used here)

				//This is additional info structure prepared by Business.dll. 
				//It contains updated objects Position, Order Execution (look in BusinessApi.h).
				//You can access objects' fields, but it is not recommended to change them (The fields are protected and you should not play any tricks to modify the fields. It will cause unpredictable results)
				AIMsgExecution* info = (AIMsgExecution*)additionalInfo;
				Order* order = info->m_order;
				const Position* position = info->m_position;
				const Execution* exec = info->m_execution;
				if ((order==NULL) || (position==NULL) || (exec==NULL)) return; // don't process null orders

				unsigned int thisid = this->cacheOrder(order);
				CString ac = CString(B_GetAccountName(position->GetAccount()));

				// build the serialized trade object
				CTime ct(msg->x_Time);
				int xd = (ct.GetYear()*10000)+(ct.GetMonth()*100)+ct.GetDay();
				int xt = (ct.GetHour()*100)+ct.GetMinute();
				TradeLibFast::TLTrade fill;
				fill.id = thisid;
				fill.xsec = ct.GetSecond();
				fill.xtime = xt;
				fill.xdate = xd;
				fill.side = (order->GetSide()=='B');
				fill.comment = CString(order->GetUserDescription());
				fill.symbol = CString(msg->x_Symbol);
				fill.xprice = (double)msg->x_ExecutionPrice/1024;
				fill.xsize= msg->x_NumberOfShares;
				fill.exchange = CString(ExchangeName((long)msg->x_executionId));
				fill.account = CString(B_GetAccountName(position->GetAccount()));

				std::vector <CString> subname;
				AllClients(subname);
				for (size_t i = 0; i< subname.size(); i++) 
					TLSend(EXECUTENOTIFY,fill.Serialize(),subname[i]);
			} // has additional info end
			break;
			case M_POOL_ASSIGN_ORDER_ID://Original order sent has a unigue generated id. The server sends this message to notify you that the order was assigned a new id different from the original. Both ids are part of this notification structure. This message can come 1 or 2 times.
			case M_POOL_UPDATE_ORDER:// Order status is modified
			if(additionalInfo != NULL && additionalInfo->GetType() == M_AI_ORDER)
			{

				AIMsgOrder* info = (AIMsgOrder*)additionalInfo;
				Order* order = info->m_order;
				const Position* position = info->m_position;

				if ((order==NULL) || (position==NULL) || (info==NULL)) return; // don't process null orders

				if (order->isDead()) return; // don't notify on dead orders

				unsigned int max = ordercache.size();
				unsigned int index = cacheOrder(order);
				if (index!=max) // if index isn't at the end, we've already notified for order
					return;


				CTime ct = CTime::GetCurrentTime();
				TLOrder o;
				o.id = index;
				o.price = order->isMarketOrder() ? 0: order->GetOrderPrice().toDouble();
				o.sec = ct.GetSecond();
				o.stop = order->GetStopPrice()->toDouble();
				o.time = (ct.GetHour()*100)+ct.GetMinute();
				o.date = (ct.GetYear()*10000)+(ct.GetMonth()*100)+ct.GetDay();
				o.size = order->GetSize();
				o.side = order->GetSide()=='B';
				o.comment = order->GetUserDescription();
				o.TIF = TIFName(order->GetTimeInForce());
				o.account = CString(B_GetAccountName(order->GetAccount()));
				o.symbol = CString(order->GetSymbol());
				std::vector <CString> subname;
				AllClients(subname);
				for (size_t i = 0; i< subname.size(); i++) 
					TLSend(ORDERNOTIFY,o.Serialize(),subname[i]);
				break;
			} // has addt info / caseend
			case M_REQ_CANCEL_ORDER:
			{
				AIMsgOrder* info = (AIMsgOrder*)additionalInfo;
				Order* order = info->m_order;
				unsigned int anvilid = order->GetId();
				unsigned int id = cacheOrder(order);
				CString msg;
				msg.Format("%u",id);
				std::vector<CString> clients;
				AllClients(clients);
				for (size_t i = 0; i<clients.size(); i++)
					TLSend(ORDERCANCELRESPONSE,msg,clients[i]);
				break;

			}
			break;
		} // switchend
	}

		unsigned int AnvilId(unsigned int TLOrderId)
	{
		if (!hasOrder(TLOrderId)) return -1;
		Order* o = ordercache[TLOrderId];
		return o->GetId();
	}



}



	void TLUnload()
	{
		for (size_t i = 0; i<subs.size(); i++)
		{
			if (subs[i]!=NULL)
			{
				subs[i]->Clear();
				delete subs[i];
				subs[i] = NULL;
			}
		}
		client.clear();
		heart.clear();
		stocks.clear();
		subs.clear();
	}

	void AllClients(std::vector <CString> &clients)
	{
		size_t count = client.size();
		for (size_t i = 0; i<count; i++)
			clients.push_back(client[i]);
	}

	void Subscribers(CString stock, std::vector <CString> &subscriberids)
	{
		size_t maxclient = stocks.size();
		for (size_t i = 0; i<maxclient; i++)
		{
			size_t maxstocks = stocks[i].size(); //numstocks for this client
			for (size_t j=0; j<maxstocks; j++) 
				if (stock.CompareNoCase(stocks[i][j])==0) // our stock matches the current stock of this client
				{
					subscriberids.push_back(client[i]); // so we save this client
					break; // no need to process more stocks for this client
				}
		}
	}



	int FindClient(CString cwind)
	{
		size_t len = client.size();
		for (size_t i = 0; i<len; i++) if (client[i]==cwind) return i;
		return -1;
	}

	LRESULT RegClient(CString m) 
	{
		if (FindClient(m)!=-1) return 1;
		client.push_back(m);
		time_t now;
		time(&now);
		heart.push_back(now); // save heartbeat at client index
		mystocklist my = mystocklist(0);
		stocks.push_back(my);
		return 0;
	}

	LRESULT HeartBeat(CString cwind)
	{
		int cid = FindClient(cwind);
		if (cid==-1) return -1;
		time_t now;
		time(&now);
		time_t then = heart[cid];
		double dif = difftime(now,then);
		heart[cid] = now;
		return (LRESULT)dif;
	}

	int LastBeat(CString cwind)
	{
		int cid = FindClient(cwind);
		if (cid==-1) return 6400000;
		time_t now,then;
		time(&now);
		then = heart[cid];
		double dif = difftime(now,then);
		if (then==0) dif = 0;
		return (int)dif;
	}


	size_t SubIdx(CString symbol)
	{
		for (size_t i = 0; i<subsym.size(); i++) 
			if (subsym[i]==symbol)
				return i;
		return -1;
	}

	bool hasHammerSub(CString symbol)
	{
		return SubIdx(symbol)!=-1;
	}


	bool isIndex(CString sym)
	{
		int slashi = sym.FindOneOf("/");
		int doli = sym.FindOneOf("$");
		int poundi = sym.FindOneOf("#");
		return (slashi!=-1)||(doli!=-1)||(poundi!=-1);
	}

	LRESULT RegStocks(CString m)
	{ 
		std::vector <CString> rec;
		gsplit(m,"+",rec); // split message body into client id and stock list
		unsigned int cid = FindClient(rec[0]);
		if (cid==-1) return 1; //client not registered
		mystocklist my; // initialize stocklist array to proper length
		gsplit(rec[1],",",my); // populate array from the message
		for (size_t i = 0; i<my.size();i++) // subscribe to stocks
		{
			if (hasHammerSub(my[i])) continue; // if we've already subscribed once, skip to next stock
			Observer* sec;
			if (isIndex(my[i]))
				sec = new TLIdx(my[i]);
			else
			{
				TLStock *stk = new TLStock(my[i]); // create new stock instance
				stk->Load();
				sec = stk;
			}
			subs.push_back(sec);
			subsym.push_back(my[i]);
		}
		stocks[cid] = my; // index the array by the client's id
		HeartBeat(rec[0]); // update the heartbeat
		return 0;
	}

	void RemoveSub(CString stock)
	{
		if (hasHammerSub(stock))
		{
			size_t i = SubIdx(stock);
			if (subs[i]!=NULL)
			{
				subs[i]->Clear();
				delete subs[i];
				subsym[i] = "";
				subs[i]= NULL;
			}
		}
	}

	void RemoveSubs(CString cwind)
	{
		int cid = FindClient(cwind);
		if (cid==-1) return;
		size_t maxstocks = stocks[cid].size();
		for (size_t i =0; i<maxstocks; i++)
		{
			std::vector <CString> ALLSUBS;
			::Subscribers(stocks[cid][i],ALLSUBS);
			if ((ALLSUBS.size()==1) // if we only have one subscriber
				&& (ALLSUBS[0].Compare(cwind)==0)) // and it's the one we requested
				RemoveSub(stocks[cid][i]);  // remove it
			// otherwise leave it
		}

	}


	LRESULT ClearClient(CString m) 
	{
		RemoveSubs(m); // remove any subscriptions this stock has that aren't used by others
		int cid = FindClient(m);
		if (cid==-1) return 1;
		client[cid] = "";
		stocks[cid] = mystocklist(0);
		heart[cid] = NULL;
		return 0;
	}

	LRESULT ClearStocks(CString m)
	{
		RemoveSubs(m);
		int cid = FindClient(m);
		if (cid==-1) return 1;
		stocks[cid] = mystocklist(0);
		HeartBeat(m);
		return 0;
	}



	void TLKillDead(int deathInseconds)
	{
		for (size_t i = 0; i<client.size(); i++) 
		{
			int lastbeat = LastBeat(client[i]);
			if ((client[i]!="") && (lastbeat>deathInseconds))
			{
				RemoveSubs(client[i]);
				ClearClient(client[i]);
			}
		}
	}

	int Sendorder(CString msg) 
	{
		if (msg=="") return GOTNULLORDER;

		TradeLibFast::TLOrder o = TradeLibFast::TLOrder::Deserialize(msg);
		const StockBase* Stock = B_GetStockHandle(o.symbol);

		Observable* m_account;
		if (o.account=="")
			m_account = B_GetCurrentAccount();
		else 
			m_account = B_GetAccount(o.account.GetBuffer());
		


		//convert the arguments
		Order* orderSent;
		char side = (o.side) ? 'B' : 'S';
		const Money pricem = Money((int)(o.price*1024));
		const Money stopm = Money((int)(o.stop*1024));
		unsigned int mytif = TIFId(o.TIF);

		if ((Stock==NULL) || (!Stock->isLoaded()))
			return UNKNOWNSYM;
		
		// send the order (next line is from SendOrderDlg.cpp)
		unsigned int error = B_SendOrder(Stock,
				side,
				o.exchange,
				o.size,
				OVM_VISIBLE, //visability mode
				o.size, //visable size
				pricem,//const Money& price,0 for Market
				&stopm,//const Money* stopPrice,
				NULL,//const Money* discrtetionaryPrice,
				mytif,
				false,//bool proactive,
				true,//bool principalOrAgency, //principal - true, agency - false
				SUMO_ALG_UNKNOWN,//char superMontageAlgorithm,
				OS_RESIZE,
				//false,//bool delayShortTillUptick,
				DE_DEFAULT, //destination exchange
				&orderSent,
				m_account,
				0,
				false,
				101, o.comment);	
		return error;
	}

	long GetPosF(CString msg, int funct) 
	{
		CString m(msg);
		Position* mypos;
		if (m.Find(",",0)==-1)
		{

			const char* symbol = (LPCTSTR)msg;
			Observable* m_account = B_GetCurrentAccount();
			mypos = B_FindPosition(symbol,m_account);
		} 
		else
		{
			std::vector<CString> parse;
			gsplit(m,",",parse);
			Observable* m_account = B_GetAccount(parse[1].GetBuffer());
			mypos = B_FindPosition(parse[0].GetBuffer(),m_account);
		}
		Money& p = Money(0,0);
		long price = 0;
		if (mypos) {
			if (funct==AVGPRICE) p = mypos->GetAveragePrice();
			else if (funct==POSOPENPL) p = mypos->GetOpenPnl();
			else if (funct==POSCLOSEDPL) p = mypos->GetClosedPnl();
			price = MoneyToPacked(p);
		}
		return price;
	}

	int GetPosI(CString msg, int funct) {
		CString m(msg);
		Position* mypos;
		if (m.Find(",",0)==-1)
		{

			const char* symbol = (LPCTSTR)msg;
			Observable* m_account = B_GetCurrentAccount();
			mypos = B_FindPosition(symbol,m_account);
		} 
		else
		{
			std::vector<CString> parse;
			gsplit(m,",",parse);
			Observable* m_account = B_GetAccount(parse[1].GetBuffer());
			mypos = B_FindPosition(parse[0].GetBuffer(),m_account);
		}
		int s = 0;
		if (mypos) {
			if (funct==POSLONGPENDSHARES) s = mypos->GetSharesPendingLong();
			else if (funct==POSSHORTPENDSHARES) s = mypos->GetSharesPendingShort();
			else if (funct==POSTOTSHARES) s = (int)mypos->GetSharesClosed();
			else if (funct==GETSIZE) s = (int)mypos->GetSize();
		}
		return s;
	}

	int GetStockI(CString msg, int funct) {
		const char* symbol = (LPCTSTR)msg;
		const StockBase* Stock = B_GetStockHandle(symbol);
		int s = 0;
		if (Stock) {
			if (funct==LASTSIZE) s = Stock->GetLastTradeSize();
		}
		return s;
	}

	long GetStockF(CString msg, int funct) {
		const char* symbol = (LPCTSTR)msg;
		const StockBase* Stock = B_GetStockHandle(symbol);
		Money& p = Money(0,0);
		long price = 0;
		if (Stock) {
			if (funct==LRPBID) p = Stock->GetLRP(true); // true = bid
			else if (funct==LRPASK) p = Stock->GetLRP(false); // false = ask
			else if (funct==LASTTRADE) p = Stock->GetLastTradePrice();
			else if (funct==NDAYHIGH) p = Stock->GetNyseDayHigh();
			else if (funct==NDAYLOW) p = Stock->GetNyseDayLow();
			else if (funct==OPENPRICE) p = Stock->GetOpenPrice();
			else if (funct==CLOSEPRICE) p = Stock->GetTodaysClosePrice();
			else if (funct==YESTCLOSE) p = Stock->GetClosePrice();
			price = MoneyToPacked(p);
		}
		return price;
	}

	LRESULT gotAccountRequest(CString client)
	{
		void* iterator = B_CreateAccountIterator();
		B_StartIteration(iterator);
		Observable* a;
		std::vector<CString> accts;
		while (a = B_GetNextAccount(iterator)) // loop through every available account
		{
			PTCHAR username = (PTCHAR)B_GetAccountName(a);
			CString un(username);
			accts.push_back(un);
		}
		B_DestroyIterator(iterator);
		CString msg = gjoin(accts,",");
		TradeLibFast::TLServer_WM::TLSend(ACCOUNTRESPONSE,msg,client);
		return OK;
	}

	CString SerializeIntVec(std::vector<int> input)
	{
		std::vector<CString> tmp;
		for (size_t i = 0; i<input.size(); i++)
		{
			CString t; // setup tmp string
			t.Format("%i",input[i]); // convert integer into tmp string
			tmp.push_back(t); // push converted string onto vector
		}
		// join vector and return serialized structure
		return gjoin(tmp,",");
	}

	int PosList(CString req)
	{
		std::vector<CString> r;
		gsplit(req,"+",r);
		if (r.size()<2) return 0;
		CString account = r[1];
		CString client = r[0];
		Observable* m_account = B_GetAccount(account);
		void* iterator = B_CreatePositionIterator(POSITION_FLAT|POSITION_LONG|POSITION_SHORT, (1 << ST_LAST) - 1,m_account);
		B_StartIteration(iterator);
		const Position* pos;
		int count = 0;
		while(pos = B_GetNextPosition(iterator))
		{
			TradeLibFast::TLPosition p;
			p.AvgPrice = pos->GetAverageExecPrice().GetMoneyValueForServer()/(double)1024;
			p.ClosedPL = pos->GetClosedPnl().GetMoneyValueForServer()/(double)1024;
			p.Size = pos->GetSize();
			p.Symbol = CString(pos->GetSymbol());
			CString msg = p.Serialize();
			TradeLibFast::TLServer_WM::TLSend(POSITIONRESPONSE,msg,client);
			count++;
		}
		B_DestroyIterator(iterator);
		m_account = NULL;
		pos = NULL;
		return count;
	}

	CString GetFeatures()
	{
		std::vector<int> f;
		f.push_back(BROKERNAME);
		f.push_back(ACCOUNTREQUEST);
		f.push_back(ACCOUNTRESPONSE);
		f.push_back(HEARTBEAT);
		f.push_back(NDAYHIGH);
		f.push_back(NDAYLOW);
		f.push_back(OPENPRICE);
		f.push_back(CLOSEPRICE);
		f.push_back(YESTCLOSE);
		f.push_back(SENDORDER);
		f.push_back(AVGPRICE);
		f.push_back(POSOPENPL);
		f.push_back(POSCLOSEDPL);
		f.push_back(POSLONGPENDSHARES);
		f.push_back(POSSHORTPENDSHARES);
		f.push_back(POSITIONREQUEST);
		f.push_back(POSITIONRESPONSE);
		f.push_back(LRPBID);
		f.push_back(LRPASK);
		f.push_back(POSTOTSHARES);
		f.push_back(LASTTRADE);
		f.push_back(LASTSIZE);
		f.push_back(REGISTERCLIENT);
		f.push_back(REGISTERSTOCK);
		f.push_back(CLEARCLIENT);
		f.push_back(CLEARSTOCKS);
		f.push_back(REGISTERINDEX);
		f.push_back(ACCOUNTOPENPL);
		f.push_back(ACCOUNTCLOSEDPL);
		f.push_back(ORDERCANCELREQUEST);
		f.push_back(ORDERCANCELRESPONSE);
		f.push_back(FEATUREREQUEST);
		f.push_back(FEATURERESPONSE);
		f.push_back(ISSIMULATION);
		f.push_back(TICKNOTIFY);
		f.push_back(TRADENOTIFY);
		f.push_back(ORDERNOTIFY);
		return SerializeIntVec(f);
	}

	LPARAM ServiceMsg(const int t,CString m) {
		
		// we need an Observer (SendOrderDlg) for stocks that aren't loaded
		if (t==BROKERNAME) return (LRESULT)Assent;
		if (t==ACCOUNTREQUEST) return gotAccountRequest(m);
		if (t==HEARTBEAT) return (LRESULT)HeartBeat(m);
		if (t==NDAYHIGH) return (LRESULT)GetStockF(m,NDAYHIGH);
		if (t==NDAYLOW) return (LRESULT)GetStockF(m,NDAYLOW);
		if (t==OPENPRICE) return (LRESULT)GetStockF(m,OPENPRICE);
		if (t==CLOSEPRICE) return (LRESULT)GetStockF(m,CLOSEPRICE);
		if (t==YESTCLOSE) return (LRESULT)GetStockF(m,YESTCLOSE);
		if (t==SENDORDER) return Sendorder(m);
		if (t==AVGPRICE) return (LRESULT)GetPosF(m,AVGPRICE);
		if (t==POSOPENPL) return (LRESULT)GetPosF(m,POSOPENPL);
		if (t==POSCLOSEDPL) return (LRESULT)GetPosF(m,POSCLOSEDPL);
		if (t==POSLONGPENDSHARES) return GetPosI(m,POSLONGPENDSHARES);
		if (t==POSSHORTPENDSHARES) return GetPosI(m,POSSHORTPENDSHARES);
		if (t==LRPBID) return (LRESULT)GetStockF(m,LRPBID);
		if (t==LRPASK)	return (LRESULT)GetStockF(m,LRPASK);
		if (t==POSTOTSHARES) return (LRESULT)GetPosI(m,POSTOTSHARES);
		if (t==LASTTRADE) return (LRESULT)GetStockF(m,LASTTRADE);
		if (t==LASTSIZE) return (LRESULT)GetStockI(m,LASTSIZE);
		if (t==GETSIZE) return (LRESULT)GetPosI(m,GETSIZE);
		if (t==REGISTERCLIENT) return(LRESULT)RegClient(m);
		if (t==REGISTERSTOCK) return (LRESULT)RegStocks(m);
		if (t==CLEARCLIENT) return (LRESULT)ClearClient(m);
		if (t==CLEARSTOCKS) return (LRESULT)ClearStocks(m);
		if (t==POSITIONREQUEST) return (LRESULT)PosList(m);
		if (t==ACCOUNTOPENPL)
		{
			if (m.GetLength()>0)
				return MoneyToPacked(B_GetAccountOpenPnl(B_GetAccount((LPCTSTR)m)));
			else 
				return MoneyToPacked(B_GetAccountOpenPnl(B_GetCurrentAccount()));
		}
		if (t==ACCOUNTCLOSEDPL)
		{
			if (m.GetLength()>0)
				return MoneyToPacked(B_GetAccountClosedPnl(B_GetAccount((LPCTSTR)m)));
			else 
				return MoneyToPacked(B_GetAccountClosedPnl(B_GetCurrentAccount()));

		}
		if (t==ORDERCANCELREQUEST) 
		{
			unsigned int TLorderId = strtoul(m.GetBuffer(),NULL,10); //get tradelink's order id
			unsigned int orderId = AnvilId(TLorderId); // get current anvil id from tradelink id
			void* iterator = B_CreateAccountIterator();
			B_StartIteration(iterator);
			Observable* acct;
			// loop through every available account, cancel any matching orders
			while (acct = B_GetNextAccount(iterator)) 
			{
				Order* order = B_FindOrder(orderId,acct);
				if(order)
				{
					order->Cancel();

				}
			}
			B_DestroyIterator(iterator);
			return OK;
		}
		if (t==FEATUREREQUEST)
		{
			SendMsg(FEATURERESPONSE,GetFeatures(),m);
			return OK;
		}

		
		if (t==ISSIMULATION) {
			Observable* m_account = B_GetCurrentAccount();
			BOOL isSim = B_IsAccountSimulation(m_account);
			return (LRESULT)isSim;
		}

		return UNKNOWNMSG;
	}