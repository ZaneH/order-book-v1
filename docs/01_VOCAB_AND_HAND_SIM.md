# Glossary

- side: the order side (buy or sell)
- tick: the minimum price movement of an instrument
  - example: if tick size is \$0.01, prices move in \$0.01 increments
- level: a collection of orders at a given tick value, crucial for
  displaying market depth (L2)
- FIFO: first-in-first-out (within a level)
- best bid/ask:
  - bid: the highest price a buyer is willing to pay for an asset
  - ask: the lowest price a seller is willing to accept for an asset
  - Think "highest bidder, lowest asking price"
- spread: the difference between the best bid and best ask
- cross: an incoming order "crosses" the best bid/ask price allowing
  the engine to immediately match
  - incoming bid\_price >= best\_ask
  - incoming ask\_price <= best\_bid
- maker price: execution price of a trade, equal to the limit price of the order resting in the order book
- taker price: price of the incoming order, not necessarily the execution price

# Hand-Simulations

## one that crosses multiple levels

    Book: (empty)

    Action: A1, sell 2 @ 101

    Book:
    [asks]
    101: (A1)2

    Action: A2, sell 4 @ 102

    Book:
    [asks]
    101: (A1)2
    102: (A2)4

    Action: B1, buy 6 @ 103 (bid >= ask, this will cross)

    Matching:
    - B1 takes A1 @ 101, 2/6 filled
    - B1 takes A2 @ 102, 6/6 filled

    Book: (empty)

## one FIFO within a level

    Book:
    [asks]
    105: A1(2)
    [bids]
    103: B1(6)

    Action: B2, buy 10 @ 103

    Book:
    [asks]
    105: A1(2)
    [bids]
    103: B1(6), B2(10)

    Action: B3, buy 1 @ 103

    Book:
    [asks]
    105: A1(2)
    [bids]
    103: B1(6), B2(10), B3(1)

    Action: A2, sell 5 @ 103 (crosses)

    Matching:
    - B1 takes A2 @ 103, 5/6 filled

    Book:
    [asks]
    105: A1(2)
    [bids]
    103: B1(1), B2(10), B3(1)

## one cancel case

    Book: (empty)

    Action: A1, sell 5 @ 110

    Book:
    [asks]
    110: A1(5)

    Action: A2, sell 10 @ 110

    Book:
    [asks]
    110: A1(5), A2(10)

    Action: Cancel A1

    Book:
    [asks]
    110: A2(10)

    Action: B1, buy 1 @ 110 (crosses)

    Matching:
    - B1 takes A2 @ 110, 1/10 filled

    Book:
    [asks]
    110: A2(9)
