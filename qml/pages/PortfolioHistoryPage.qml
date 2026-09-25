import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property var history
    property var prices
    property string portfolioLabel: "Development wallet"

    function formatSigned(value) {
        if (value < 0.0)
            return "-" + prices.formatFiat(-value)
        if (value > 0.0)
            return "+" + prices.formatFiat(value)
        return prices.formatFiat(0.0)
    }

    function formatPercent(value) {
        var prefix = value > 0.0 ? "+" : ""
        return prefix + value.toFixed(2) + "%"
    }

    function allocation(value) {
        if (history.latestTotalValue <= 0.0)
            return "0.0%"
        return ((value / history.latestTotalValue) * 100.0).toFixed(1) + "%"
    }

    function seriesName() {
        if (!history)
            return "Portfolio total"
        if (history.analysisSeries === "bitcoin")
            return "Bitcoin value"
        if (history.analysisSeries === "ethereum")
            return "Ethereum value"
        if (history.analysisSeries === "solana")
            return "Solana value"
        return "Portfolio total"
    }

    function windowName() {
        if (!history || history.analysisWindowDays === 0)
            return "All saved history"
        if (history.analysisWindowDays === 1)
            return "Last 24 hours"
        return "Last " + history.analysisWindowDays + " days"
    }

    Connections {
        target: history
        onStateChanged: historyChart.requestPaint()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            visible: history && history.count > 0

            MenuItem {
                text: "Clear " + (history ? history.fiatCurrency : "") + " history"
                onClicked: remorse.execute(
                    "Clearing local portfolio history",
                    function() { history.clearHistory() })
            }
        }

        RemorsePopup {
            id: remorse
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Portfolio history"
                description: portfolioLabel + " · local snapshots"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: history ? history.status : ""
                color: Theme.secondaryColor
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Column {
                width: parent.width
                spacing: Theme.paddingMedium
                visible: history && history.count > 0

                SectionHeader {
                    text: "Latest native value"
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: prices && history
                          ? prices.formatFiat(history.latestTotalValue)
                          : "—"
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeHuge
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: history ? history.latestTimestamp : ""
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                }

                DetailItem {
                    label: "Change across saved history"
                    value: history && history.count > 1
                           ? formatSigned(history.changeValue)
                             + " · " + formatPercent(history.changePercent)
                           : "Need at least 2 snapshots"
                }

                SectionHeader {
                    text: "Latest allocation"
                }

                DetailItem {
                    label: "Bitcoin"
                    value: prices && history
                           ? prices.formatFiat(history.latestBitcoinValue)
                             + " · " + allocation(history.latestBitcoinValue)
                           : "—"
                }

                DetailItem {
                    label: "Ethereum"
                    value: prices && history
                           ? prices.formatFiat(history.latestEthereumValue)
                             + " · " + allocation(history.latestEthereumValue)
                           : "—"
                }

                DetailItem {
                    label: "Solana"
                    value: prices && history
                           ? prices.formatFiat(history.latestSolanaValue)
                             + " · " + allocation(history.latestSolanaValue)
                           : "—"
                }

                SectionHeader {
                    text: "History analytics"
                }

                ComboBox {
                    id: seriesBox
                    property bool settingsLoaded: false
                    width: parent.width
                    label: "Chart series"

                    menu: ContextMenu {
                        MenuItem { text: "Portfolio total" }
                        MenuItem { text: "Bitcoin value" }
                        MenuItem { text: "Ethereum value" }
                        MenuItem { text: "Solana value" }
                    }

                    Component.onCompleted: {
                        if (!history)
                            return
                        if (history.analysisSeries === "bitcoin")
                            currentIndex = 1
                        else if (history.analysisSeries === "ethereum")
                            currentIndex = 2
                        else if (history.analysisSeries === "solana")
                            currentIndex = 3
                        else
                            currentIndex = 0
                        settingsLoaded = true
                    }

                    onCurrentIndexChanged: {
                        if (!history || !settingsLoaded)
                            return
                        var series = "total"
                        if (currentIndex === 1) series = "bitcoin"
                        else if (currentIndex === 2) series = "ethereum"
                        else if (currentIndex === 3) series = "solana"
                        if (history.analysisSeries !== series)
                            history.analysisSeries = series
                    }
                }

                ComboBox {
                    id: windowBox
                    property bool settingsLoaded: false
                    width: parent.width
                    label: "History window"

                    menu: ContextMenu {
                        MenuItem { text: "All saved history" }
                        MenuItem { text: "Last 24 hours" }
                        MenuItem { text: "Last 7 days" }
                        MenuItem { text: "Last 30 days" }
                    }

                    Component.onCompleted: {
                        if (!history)
                            return
                        if (history.analysisWindowDays === 1)
                            currentIndex = 1
                        else if (history.analysisWindowDays === 7)
                            currentIndex = 2
                        else if (history.analysisWindowDays === 30)
                            currentIndex = 3
                        else
                            currentIndex = 0
                        settingsLoaded = true
                    }

                    onCurrentIndexChanged: {
                        if (!history || !settingsLoaded)
                            return
                        var days = 0
                        if (currentIndex === 1) days = 1
                        else if (currentIndex === 2) days = 7
                        else if (currentIndex === 3) days = 30
                        if (history.analysisWindowDays !== days)
                            history.analysisWindowDays = days
                    }
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: history ? history.analysisStatus : ""
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }

                Canvas {
                    id: historyChart
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    height: Theme.itemSizeHuge * 2.2
                    visible: history && history.analysisCount > 0

                    Component.onCompleted: requestPaint()

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        if (!history || history.analysisCount < 1)
                            return

                        var points = history.analysisEntries
                        var left = Theme.paddingMedium
                        var right = width - Theme.paddingMedium
                        var top = Theme.paddingMedium
                        var bottom = height - Theme.paddingMedium
                        var plotWidth = Math.max(1, right - left)
                        var plotHeight = Math.max(1, bottom - top)
                        var minValue = history.analysisLowValue
                        var maxValue = history.analysisHighValue

                        if (Math.abs(maxValue - minValue) < 0.0000001) {
                            if (maxValue <= 0.0) {
                                minValue = 0.0
                                maxValue = 1.0
                            } else {
                                var pad = Math.max(maxValue * 0.02, 0.01)
                                minValue = Math.max(0.0, minValue - pad)
                                maxValue += pad
                            }
                        }

                        ctx.save()
                        ctx.globalAlpha = 0.25
                        ctx.strokeStyle = Theme.secondaryColor
                        ctx.lineWidth = 1
                        for (var grid = 0; grid < 3; ++grid) {
                            var gy = top + (plotHeight * grid / 2.0)
                            ctx.beginPath()
                            ctx.moveTo(left, gy)
                            ctx.lineTo(right, gy)
                            ctx.stroke()
                        }
                        ctx.restore()

                        var firstTime = points[0].recordedEpoch
                        var lastTime = points[points.length - 1].recordedEpoch

                        function chartX(point, index) {
                            if (points.length <= 1 || lastTime <= firstTime)
                                return points.length <= 1
                                     ? left + plotWidth / 2.0
                                     : left + plotWidth * index / (points.length - 1)
                            return left + plotWidth
                                   * (point.recordedEpoch - firstTime)
                                   / (lastTime - firstTime)
                        }

                        function chartY(value) {
                            return bottom - ((value - minValue) / (maxValue - minValue)) * plotHeight
                        }

                        ctx.strokeStyle = Theme.highlightColor
                        ctx.fillStyle = Theme.highlightColor
                        ctx.lineWidth = 3
                        ctx.beginPath()
                        for (var i = 0; i < points.length; ++i) {
                            var x = chartX(points[i], i)
                            var y = chartY(points[i].value)
                            if (i === 0)
                                ctx.moveTo(x, y)
                            else
                                ctx.lineTo(x, y)
                        }
                        ctx.stroke()

                        for (var j = 0; j < points.length; ++j) {
                            var px = chartX(points[j], j)
                            var py = chartY(points[j].value)
                            ctx.fillRect(px - 2, py - 2, 4, 4)
                        }
                    }
                }

                DetailItem {
                    visible: history && history.analysisCount > 0
                    label: seriesName() + " range"
                    value: windowName()
                }

                DetailItem {
                    visible: history && history.analysisCount > 0
                    label: "Low / high"
                    value: prices
                           ? prices.formatFiat(history.analysisLowValue)
                             + " / " + prices.formatFiat(history.analysisHighValue)
                           : "—"
                }

                DetailItem {
                    visible: history && history.analysisCount > 0
                    label: "Average"
                    value: prices ? prices.formatFiat(history.analysisAverageValue) : "—"
                }

                DetailItem {
                    visible: history && history.analysisCount > 1
                    label: "Change in selected window"
                    value: formatSigned(history.analysisChangeValue)
                           + " · "
                           + (history.analysisStartValue > 0.0
                              ? formatPercent(history.analysisChangePercent)
                              : "percentage n/a")
                }

                DetailItem {
                    visible: history && history.analysisCount > 0
                    label: "First / latest point"
                    value: history.analysisStartTimestamp
                           + " / " + history.analysisEndTimestamp
                }

                SectionHeader {
                    text: "Snapshots · newest first"
                }

                Repeater {
                    model: history ? history.entries : []

                    Item {
                        width: content.width
                        height: snapshotColumn.height + 2 * Theme.paddingMedium

                        Column {
                            id: snapshotColumn
                            x: Theme.horizontalPageMargin
                            y: Theme.paddingMedium
                            width: parent.width - 2 * Theme.horizontalPageMargin
                            spacing: Theme.paddingSmall

                            Label {
                                width: parent.width
                                text: modelData.recordedAt
                                color: Theme.primaryColor
                                font.pixelSize: Theme.fontSizeMedium
                            }

                            Label {
                                width: parent.width
                                text: prices.formatFiat(modelData.totalValue)
                                color: Theme.highlightColor
                                font.pixelSize: Theme.fontSizeLarge
                            }

                            Label {
                                width: parent.width
                                text: "BTC " + prices.formatFiat(modelData.bitcoinValue)
                                      + " · ETH " + prices.formatFiat(modelData.ethereumValue)
                                      + " · SOL " + prices.formatFiat(modelData.solanaValue)
                                color: Theme.secondaryColor
                                font.pixelSize: Theme.fontSizeExtraSmall
                                wrapMode: Text.Wrap
                            }

                            Label {
                                width: parent.width
                                text: "Balances: BTC " + modelData.bitcoinBalance
                                      + " · ETH " + modelData.ethereumBalance
                                      + " · SOL " + modelData.solanaBalance
                                color: Theme.secondaryColor
                                font.pixelSize: Theme.fontSizeExtraSmall
                                wrapMode: Text.Wrap
                            }
                        }

                        Separator {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            color: Theme.secondaryColor
                        }
                    }
                }
            }

            Column {
                width: parent.width
                spacing: Theme.paddingMedium
                visible: history && history.count === 0

                SectionHeader {
                    text: "How history is recorded"
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: "SailVault records a local snapshot after a successful balance "
                          + "and market-price refresh. Cached data loaded at startup does "
                          + "not create a new history point."
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.Wrap
                }
            }

            SectionHeader {
                text: "Privacy & scope"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Opening this page performs no network request. The chart, range "
                      + "statistics and snapshot list are calculated locally from native "
                      + "BTC, ETH and SOL history already stored on this device. Token "
                      + "holdings are not included. A maximum of 120 snapshots is retained "
                      + "for this public-address portfolio context."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }
    }
}
