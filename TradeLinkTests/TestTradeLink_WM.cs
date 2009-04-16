using System;
using System.Collections.Generic;
using System.Text;
using NUnit.Framework;
using TradeLink.Common;
using System.Windows.Forms;
using TradeLink.API;


namespace TestTradeLink
{
    [TestFixture]
    public class TestTradeLink_WM
    {
        // each side of our "link"
        TLClient_WM c;
        TLClient_WM c2;
        TLServer_WM s;

        // counters used to test link events are working
        int ticks;
        int fills;
        int orders;
        int fillrequest;
        int imbalances;

        // 2nd client counters
        int copyticks;
        int copyfills;
        int copyorders;

        const string SYM = "TST";

        public TestTradeLink_WM() 
        {
            s = new TLServer_WM();
            c = new TLClient_WM("testtradelink",false);
            // create a second client to verify order and fill copying work
            c2 = new TLClient_WM("client2", false);

            // register server events (so server can process orders)
            s.newSendOrderRequest += new OrderDelegate(tl_gotSrvFillRequest);

            // setup client events
            c.gotFill += new FillDelegate(tlclient_gotFill);
            c.gotOrder += new OrderDelegate(tlclient_gotOrder);
            c.gotTick += new TickDelegate(tlclient_gotTick);
            c.gotImbalance += new ImbalanceDelegate(c_gotImbalance);
            // setup second client events to check copying
            c2.gotFill += new FillDelegate(c2_gotFill);
            c2.gotOrder += new OrderDelegate(c2_gotOrder);
            c2.gotTick += new TickDelegate(c2_gotTick);

        }

        void c_gotImbalance(Imbalance imb)
        {
            imbalances++;
        }

        [Test]
        public void Imbalance()
        {
            // make sure we have none sent
            Assert.AreEqual(0, imbalances);

            // send one
            s.newImbalance(new ImbalanceImpl(SYM, "NYSE", 100000, 154000, 0, 0));

            // make sure we got it
            Assert.AreEqual(1, imbalances);

            
        }



        [Test]
        public void StartupTests()
        {
            // we're expecting this server type
            TLTypes expect = TLTypes.HISTORICALBROKER;
            // discover our states
            TLTypes FOUND = c.TLFound();
            bool CONNECTED = c.Mode(FOUND&expect,true,false);

            // should be able to connect to whatever server we find
            Assert.That(CONNECTED,"make sure you don't have TLServers running");

        }

        [Test]
        public void TickTests()
        {
            // havent' sent any ticks, so shouldn't have any counted
            Assert.That(ticks == 0, ticks.ToString());

            // have to subscribe to a stock to get notified on fills for said stock
            c.Subscribe(new BasketImpl(new SecurityImpl(SYM)));

            //send a tick from the server
            TickImpl t = TickImpl.NewTrade(SYM, 10, 100);
            s.newTick(t);

            // make sure the client got it
            Assert.That(ticks == 1, ticks.ToString());
            // make sure other clients did not get ticks 
            // (cause we didnt' subscribe from other clients)
            Assert.AreNotEqual(copyticks, ticks);
        }

        [Test]
        public void OrderTests()
        {
            // no orders yet
            Assert.That(orders == 0, orders.ToString());
            // no fill requests yet
            Assert.That(fillrequest == 0, fillrequest.ToString());

            // client wants to buy 100 TST at market
            OrderImpl o = new OrderImpl(SYM, 100);
            // if it works it'll return zero
            int error = c.SendOrder(o);
            Assert.That(error==0,error.ToString());
            // client should have received notification that an order entered his account
            Assert.That(orders == 1, orders.ToString());
            // server should have gotten a request to fill an order
            Assert.That(fillrequest == 1, fillrequest.ToString());
            // make sure order was copied to other clients
            Assert.AreEqual(copyorders, orders);
        }

        [Test]
        public void FillTests()
        {
            // no executions yet
            Assert.That(fills == 0, fills.ToString());

            // have to subscribe to a stock to get notified on fills for said stock
            c.Subscribe(new BasketImpl(new SecurityImpl(SYM)));

            // prepare and send an execution from client to server
            TradeImpl t = new TradeImpl(SYM, 100, 300, DateTime.Now);
            s.newFill(t);

            // make sure client received and counted it
            Assert.That(fills == 1, fills.ToString());

            // make sure fill was copied
            Assert.AreEqual(fills, copyfills);
        }

        [Test]
        public void PerformanceTests()
        {
            // expected performance
            const decimal EXPECT = .10m;
            // get ticks for test
            const int TICKSENT = 1000;
            Tick[] tick = TradeLink.Research.RandomTicks.GenerateSymbol(SYM, TICKSENT);
            // subscribe to symbol
            c.Unsubscribe();
            c.Subscribe(new BasketImpl(SYM));
            // reset ticks
            int save = ticks;
            ticks = 0;
            // start clock
            DateTime start = DateTime.Now;

            // process ticks
            for (int i = 0; i < tick.Length; i++)
                s.newTick(tick[i]);

            // stop clock
            double time = DateTime.Now.Subtract(start).TotalSeconds;
            // make sure time exists
            Assert.Greater(time, 0);
            // make sure it's less than expected
            Assert.LessOrEqual(time, EXPECT);
            // make sure we got all the ticks
            Assert.AreEqual(TICKSENT, ticks);
            decimal ticksec = TICKSENT/(decimal)time;
            Console.WriteLine("protocol performance (tick/sec): " + ticksec.ToString("N0"));

            // restore ticks
            ticks = save;
        }

        // event handlers

        void tl_gotSrvFillRequest(Order o)
        {
            s.newOrder(o);
            fillrequest++;
        }

        void tlclient_gotTick(Tick t)
        {
            ticks++;

        }

        void tlclient_gotOrder(Order o)
        {
            orders++;
        }

        void tlclient_gotFill(Trade t)
        {
            fills++;
        }

        // 2nd client handlers
        void c2_gotTick(Tick t)
        {
            copyticks++;
        }

        void c2_gotOrder(Order o)
        {
            copyorders++; 
        }

        void c2_gotFill(Trade t)
        {
            copyfills++;
        }
    }
}
