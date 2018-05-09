#ifndef CLIENTMODEL_H
#define CLIENTMODEL_H

#include <QObject>
#include <QDateTime>

class OptionsModel;
class AddressTableModel;
class TransactionTableModel;
class PeerTableModel;
class BanTableModel;
class CWallet;

QT_BEGIN_NAMESPACE
class QDateTime;
class QTimer;
QT_END_NAMESPACE

/** Model for Bitcoin network client. */
class ClientModel : public QObject
{
    Q_OBJECT
public:
    explicit ClientModel(OptionsModel *optionsModel, QObject *parent = 0);
    ~ClientModel();

    OptionsModel *getOptionsModel();
    PeerTableModel *getPeerTableModel();
    BanTableModel *getBanTableModel();

    int getNumConnections() const;
    int getNumBlocks() const;
    int getNumBlocksAtStartup();
    double getPosKernalPS();
    int getStakeTargetSpacing();

    QDateTime getLastBlockDate() const;

	// by Simone: necessary for network chart
    quint64 getTotalBytesRecv() const;
    quint64 getTotalBytesSent() const;

    //! Return true if client connected to testnet
    bool isTestNet() const;
    //! Return true if core is doing initial block download
    bool inInitialBlockDownload() const;
    //! Return conservative estimate of total number of blocks, or 0 if unknown
    int getNumBlocksOfPeers() const;
    //! Return warnings to be displayed in status bar
    QString getStatusBarWarnings() const;

    QString formatFullVersion() const;
    QString formatBuildDate() const;
    QString clientName() const;
    QString formatClientStartupTime() const;
	double GetDifficulty() const;

private:
    OptionsModel *optionsModel;
    BanTableModel *banTableModel;
	PeerTableModel *peerTableModel;

    int cachedNumBlocks;
    int cachedNumBlocksOfPeers;

    QTimer *pollTimer;

    int numBlocksAtStartup;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
signals:
    void numConnectionsChanged(int count);
    void numBlocksChanged(int count, int countOfPeers);
    void bytesChanged(quint64 totalBytesIn, quint64 totalBytesOut);

    //! Asynchronous error notification
    void error(const QString &title, const QString &message, bool modal);

public slots:
    void updateTimer();
    void updateNumConnections(int numConnections);
    void updateAlert(const QString &hash, int status);
};

#endif // CLIENTMODEL_H
