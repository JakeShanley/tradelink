using System;
using TradeLink.API;

namespace TradeLink.Common
{
    /// <summary>
    /// A tick is both the smallest unit of time and the most simple unit of data in TradeLink (and the markets)
    /// It is an abstract container for last trade, last trade size, best bid, best offer, bid and offer sizes.
    /// </summary>
    [Serializable]
    public struct TickImpl : TradeLink.API.Tick
    {
        public string symbol { get { return _sym; } set { _sym = value; } }
        public Security Sec { get { return _Sec; } set { _Sec = value; } }
        public int size { get { return _size; } set { _size = value; } }
        public int date { get { return _date; } set { _date = value; } }
        public int time { get { return _time; } set { _time = value; } }
        public int sec { get { return _sec; } set { _sec = value; } }
        public int bs { get { return _bs; } set { _bs = value; } }
        public int os { get { return _os; } set { _os = value; } }
        public decimal trade { get { return _trade; } set { _trade = value; } }
        public decimal bid { get { return _bid; } set { _bid = value; } }
        public decimal ask { get { return _ask; } set { _ask = value; } }
        public string ex { get { return _ex; } set { _ex = value; } }
        public string be { get { return _be; } set { _be = value; } }
        public string oe { get { return _oe; } set { _oe = value; } }
        public bool isIndex { get { return _size < 0; } }
        public bool hasBid { get { return (_bid != 0) && (_bs != 0); } }
        public bool hasAsk { get { return (_ask != 0) && (_os != 0); } }
        public bool isFullQuote { get { return hasBid && hasAsk; } }
        public bool isQuote { get { return (!isTrade && (hasBid || hasAsk)); } }
        public bool isTrade { get { return (_trade != 0) && (_size> 0); } }
        public bool hasTick { get { return (this.isTrade || hasBid || hasAsk); } }
        public bool isValid { get { return (_sym!= "") && (isIndex || hasTick); } }
        public bool atHigh(decimal high) { return (isTrade && (_trade>=high)); }
        public bool atLow(decimal low) { return (isTrade && (_trade <= low)); }
        public int BidSize { get { return _bs * 100; } set { _bs = (int)(value / 100); } }
        public int AskSize { get { return _os * 100; } set { _os = (int)(value / 100); } }
        public int TradeSize { get { return ts*100; } set { _size = (int)(value / 100); } }
        public int ts { get { return _size / 100; } } // normalized to bs/os
        Security _Sec;
        string _sym;
        string _be;
        string _oe;
        string _ex;
        int _bs;
        int _os;
        int _size;
        int _sec;
        int _date;
        int _time;
        decimal _trade;
        decimal _bid;
        decimal _ask;

        public TickImpl(string symbol) 
        {
            _Sec = new SecurityImpl(symbol);
            _sym = symbol;
            _be = "";
            _oe = "";
            _ex = "";
            _bs = 0;
            _os = 0;
            _size = 0;
            _date = 0;
            _time = 0;
            _sec = 0;
            _trade = 0m;
            _bid = 0;
            _ask = 0;
        }
        public static TickImpl Copy(Tick c)
        {
            TickImpl k = new TickImpl();
            if (c.symbol != "") k.symbol = c.symbol;
            k.time = c.time;
            k.date = c.date;
            k.sec = c.sec;
            k.size = c.size;
            k.trade = c.trade;
            k.bid = c.bid;
            k.ask = c.ask;
            k.bs = c.bs;
            k.os = c.os;
            k.be = c.be;
            k.oe = c.oe;
            k.ex = c.ex;
            return k;
        }
        /// <summary>
        /// this constructor creates a new tick by combining two ticks
        /// this is to handle tick updates that only provide bid/ask changes.
        /// </summary>
        /// <param name="a">old tick</param>
        /// <param name="b">new tick or update</param>
        public static Tick Copy(TickImpl a, TickImpl b)
        {
            TickImpl k = new TickImpl();
            if (b.symbol != a.symbol) return k; // don't combine different symbols
            if (b.time < a.time) return k; // don't process old updates
            k.time = b.time;
            k.date = b.date;
            k.sec = b.sec;
            k.symbol = b.symbol;

            if (b.isTrade)
            {
                k.trade = b.trade;
                k.size = b.size;
                k.ex = b.ex;
                //
                k.bid = a.bid;
                k.ask = a.ask;
                k.os = a.os;
                k.bs = a.bs;
                k.be = a.be;
                k.oe = a.oe;
            }
            else if (b.hasAsk && b.hasBid)
            {
                k.bid = b.bid;
                k.ask = b.ask;
                k.bs = b.bs;
                k.os = b.os;
                k.be = b.be;
                k.oe = b.oe;
                //
                k.trade = a.trade;
                k.size = a.size;
                k.ex = a.ex;
            }
            else if (b.hasAsk)
            {
                k.ask = b.ask;
                k.os = b.os;
                k.oe = b.oe;
                //
                k.bid = a.bid;
                k.bs = a.bs;
                k.be = a.be;
                k.trade = a.trade;
                k.size = a.size;
                k.ex = a.ex;
            }
            else if (b.hasBid)
            {
                k.bid = b.bid;
                k.bs = b.bs;
                k.be = b.be;
                //
                k.ask = a.ask;
                k.os = a.os;
                k.oe = a.oe;
                k.trade = a.trade;
                k.size = a.size;
                k.ex = a.ex;
            }
            return k;
        }

        public override string ToString()
        {
            if (!this.hasTick) return "";
            if (this.isTrade) return symbol+" "+this.size + "@" + this.trade + " " + this.ex;
            else return symbol+" "+this.bid + "x" + this.ask + " (" + this.bs + "x" + this.os + ") " + this.be + "," + this.oe;
        }

        public static string Serialize(Tick t)
        {
            const char d = ',';
            string secname = t.Sec.isValid ? t.Sec.FullName : t.symbol;
            string s = secname + d + t.date + d + t.time + d + t.sec + d + t.trade + d + t.size + d + t.ex + d + t.bid + d + t.ask + d + t.bs + d + t.os + d + t.be + d + t.oe + d;
            return s;
        }

        public static Tick Deserialize(string msg)
        {
            string [] r = msg.Split(',');
            Tick t = new TickImpl();
            t.Sec = SecurityImpl.Parse(r[(int)TickField.symbol]);
            t.symbol = t.Sec.Symbol;
            t.trade = Convert.ToDecimal(r[(int)TickField.trade]);
            t.size = Convert.ToInt32(r[(int)TickField.tsize]);
            t.bid = Convert.ToDecimal(r[(int)TickField.bid]);
            t.ask = Convert.ToDecimal(r[(int)TickField.ask]);
            t.os = Convert.ToInt32(r[(int)TickField.asksize]);
            t.bs = Convert.ToInt32(r[(int)TickField.bidsize]);
            t.ex = r[(int)TickField.tex];
            t.be = r[(int)TickField.bidex];
            t.oe = r[(int)TickField.askex];
            t.time = Convert.ToInt32(r[(int)TickField.time]);
            t.sec = Convert.ToInt32(r[(int)TickField.sec]);
            t.date = Convert.ToInt32(r[(int)TickField.date]);
            return t;
        }

        public void SetQuote(int date, int time, int sec, decimal bid, decimal ask, int bidsize, int asksize, string bidex, string askex)
        {
        	this.date = date;
        	this.time = time;
        	this.sec = sec;
        	this.bid = bid;
        	this.ask = ask;
        	this.bs = bidsize;
        	this.os = asksize;
        	this.be = bidex;
        	this.oe = askex;
        	this.trade =0;
        	this.size = 0;
        }
        //date, time, sec, Convert.ToDecimal(r[(int)T.PRICE]), isize, r[(int)T.EXCH]
        public void SetTrade(int date, int time, int sec, decimal price, int size, string exch)
        {
        	this.ex = exch;
        	this.date = date;
        	this.time = time;
        	this.sec = sec;
        	this.trade = price;
        	this.size = size;
        	this.bid = 0;
        	this.ask = 0;
        	this.os = 0;
        	this.bs = 0;
        }


        public static TickImpl NewBid(string sym, decimal bid, int bidsize) { return NewQuote(sym, 0, 0, 0, bid, 0, bidsize, 0, "", ""); }
        public static TickImpl NewAsk(string sym, decimal ask, int asksize) { return NewQuote(sym, 0, 0, 0, 0, ask, 0, asksize, "", ""); }
        public static TickImpl NewQuote(string sym, int date, int time, int sec, decimal bid, decimal ask, int bidsize, int asksize, string be, string oe)
        {
            TickImpl q = new TickImpl(sym);
            q.date = date;
            q.time = time;
            q.sec = sec;
            q.bid = bid;
            q.ask = ask;
            q.be = be.Trim();
            q.oe = oe.Trim();
            q.AskSize = asksize;
            q.BidSize= bidsize;
            q.trade = 0;
            q.size = 0;
            return q;
        }

        public static TickImpl NewTrade(string sym, decimal trade, int size) { return NewTrade(sym, 0, 0, 0, trade, size, ""); }
        public static TickImpl NewTrade(string sym, int date, int time, int sec, decimal trade, int size, string ex)
        {
            TickImpl t = new TickImpl(sym);
            t.date = date;
            t.time = time;
            t.sec = sec;
            t.trade = trade;
            t.size = size;
            t.ex = ex.Trim();
            t.bid = 0;
            return t;
        }



    }

    enum TickField
    { // tick message fields from TL server
        symbol = 0,
        date,
        time,
        sec,
        trade,
        tsize,
        tex,
        bid,
        ask,
        bidsize,
        asksize,
        bidex,
        askex,
    }
}
