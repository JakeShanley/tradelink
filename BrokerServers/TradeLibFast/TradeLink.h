#pragma once

	const CString LIVEWINDOW = _T("TL-BROKER-LIVE");
	const CString SIMWINDOW = _T("TL-BROKER-SIMU");
	const CString HISTWINDOW = _T("TL-BROKER-HIST");
	const CString TESTWINDOW = _T("TL-BROKER-TEST");

	enum TLTypes
	{
		NONE = 0,
        SIMBROKER = 2,
        LIVEBROKER = 4,
        HISTORICALBROKER = 8,
        TESTBROKER = 16,
	};

	enum TL2 {
		OK = 0,
		SENDORDER = 1,
		AVGPRICE,
		POSOPENPL,
		POSCLOSEDPL,
		POSLONGPENDSHARES,
		POSSHORTPENDSHARES,
		LRPBID,
		LRPASK,
		POSTOTSHARES,
		LASTTRADE,
		LASTSIZE,
		NDAYHIGH,
		NDAYLOW,
		INTRADAYHIGH,
		INTRADAYLOW,
		OPENPRICE,
		CLOSEPRICE,
		NLASTTRADE = 20,
		NBIDSIZE,
		NASKSIZE,
		NBID,	
		NASK,	
		ISSIMULATION,
		GETSIZE,
		YESTCLOSE,
		BROKERNAME,
		ACCOUNTOPENPL,
		ACCOUNTCLOSEDPL,
		VERSION,
		TICKNOTIFY = 100,
		EXECUTENOTIFY,
		REGISTERCLIENT,
		REGISTERSTOCK,
		CLEARSTOCKS,
		CLEARCLIENT,
		HEARTBEAT,
		ORDERNOTIFY,
		INFO,
		QUOTENOTIFY,
		TRADENOTIFY,
		REGISTERINDEX,
		DAYRANGE,
		REGISTERFUTURE,
		ACCOUNTRESPONSE = 500,
		ACCOUNTREQUEST,
		ORDERCANCELREQUEST,
		ORDERCANCELRESPONSE,
		FEATUREREQUEST,
		FEATURERESPONSE,
		POSITIONREQUEST,
		POSITIONRESPONSE,
		FEATURE_NOT_IMPLEMENTED = 994,
		CLIENTNOTREGISTERED = 995,
		GOTNULLORDER = 996,
		UNKNOWNMSG,
		UNKNOWNSYM,
		TL_CONNECTOR_MISSING,
	};

	enum Brokers
	{
		UnknownBroker = -1,
		TradeLinkSimulation = 0,
		Assent,
		InteractiveBrokers,
		Genesis,
		Bright,
		Echo,
	};




