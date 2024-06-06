#ifndef DATABASE_H
#define DATABASE_H

#include "embllm.h" // IWYU pragma: keep

#include <QElapsedTimer>
#include <QFileInfo>
#include <QLatin1String>
#include <QList>
#include <QMap>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QThread>
#include <QVector>
#include <QtGlobal>
#include <QtSql>

#include <cstddef>

class EmbeddingLLM;
class Embeddings;
class QFileSystemWatcher;
class QSqlError;
class QTextStream;
class QTimer;

struct DocumentInfo
{
    int folder;
    QFileInfo doc;
    int currentPage = 0;
    size_t currentPosition = 0;
    bool currentlyProcessing = false;
    bool isPdf() const {
        return doc.suffix() == QLatin1String("pdf");
    }
};

struct ResultInfo {
    QString file;   // [Required] The name of the file, but not the full path
    QString title;  // [Optional] The title of the document
    QString author; // [Optional] The author of the document
    QString date;   // [Required] The creation or the last modification date whichever is latest
    QString text;   // [Required] The text actually used in the augmented context
    int page = -1;  // [Optional] The page where the text was found
    int from = -1;  // [Optional] The line number where the text begins
    int to = -1;    // [Optional] The line number where the text ends
};

struct CollectionItem {
    QString collection;
    QString folder_path;
    int folder_id = -1;
    bool installed = false;
    bool indexing = false;
    QString error;
    int currentDocsToIndex = 0;
    int totalDocsToIndex = 0;
    size_t currentBytesToIndex = 0;
    size_t totalBytesToIndex = 0;
    size_t currentEmbeddingsToIndex = 0;
    size_t totalEmbeddingsToIndex = 0;
};
Q_DECLARE_METATYPE(CollectionItem)

class Database : public QObject
{
    Q_OBJECT
public:
    Database(int chunkSize);
    virtual ~Database();

public Q_SLOTS:
    void start();
    void scanQueue();
    void scanDocuments(int folder_id, const QString &folder_path, bool isNew);
    bool addFolder(const QString &collection, const QString &path, bool fromDb);
    void removeFolder(const QString &collection, const QString &path);
    void retrieveFromDB(const QList<QString> &collections, const QString &text, int retrievalSize, QList<ResultInfo> *results);
    void cleanDB();
    void changeChunkSize(int chunkSize);

Q_SIGNALS:
    void docsToScanChanged();
    void updateInstalled(int folder_id, bool b);
    void updateIndexing(int folder_id, bool b);
    void updateError(int folder_id, const QString &error);
    void updateCurrentDocsToIndex(int folder_id, size_t currentDocsToIndex);
    void updateTotalDocsToIndex(int folder_id, size_t totalDocsToIndex);
    void subtractCurrentBytesToIndex(int folder_id, size_t subtractedBytes);
    void updateCurrentBytesToIndex(int folder_id, size_t currentBytesToIndex);
    void updateTotalBytesToIndex(int folder_id, size_t totalBytesToIndex);
    void updateCurrentEmbeddingsToIndex(int folder_id, size_t currentBytesToIndex);
    void updateTotalEmbeddingsToIndex(int folder_id, size_t totalBytesToIndex);
    void addCollectionItem(const CollectionItem &item, bool fromDb);
    void removeFolderById(int folder_id);
    void collectionListUpdated(const QList<CollectionItem> &collectionList);

private Q_SLOTS:
    void directoryChanged(const QString &path);
    bool addFolderToWatch(const QString &path);
    bool removeFolderFromWatch(const QString &path);
    int addCurrentFolders();
    void handleEmbeddingsGenerated(const QVector<EmbeddingResult> &embeddings);
    void handleErrorGenerated(int folder_id, const QString &error);

private:
    enum class FolderStatus { Started, Embedding, Complete };
    struct FolderStatusRecord { qint64 startTime; bool isNew; int numDocs, docsChanged, chunksRead; };

    void removeFolderInternal(const QString &collection, int folder_id, const QString &path);
    size_t chunkStream(QTextStream &stream, int folder_id, int document_id, const QString &file,
        const QString &title, const QString &author, const QString &subject, const QString &keywords, int page,
        int maxChunks = -1);
    void removeEmbeddingsByDocumentId(int document_id);
    void scheduleNext(int folder_id, size_t countForFolder);
    void handleDocumentError(const QString &errorMessage,
        int document_id, const QString &document_path, const QSqlError &error);
    size_t countOfDocuments(int folder_id) const;
    size_t countOfBytes(int folder_id) const;
    DocumentInfo dequeueDocument();
    void removeFolderFromDocumentQueue(int folder_id);
    void enqueueDocumentInternal(const DocumentInfo &info, bool prepend = false);
    void enqueueDocuments(int folder_id, const QVector<DocumentInfo> &infos);
    void updateIndexingStatus();
    void updateFolderStatus(int folder_id, FolderStatus status, int numDocs = -1, bool atStart = false, bool isNew = false);

private:
    int m_chunkSize;
    QTimer *m_scanTimer;
    QMap<int, QQueue<DocumentInfo>> m_docsToScan;
    QElapsedTimer m_indexingTimer;
    QMap<int, FolderStatusRecord> m_foldersBeingIndexed;
    QList<ResultInfo> m_retrieve;
    QThread m_dbThread;
    QFileSystemWatcher *m_watcher;
    EmbeddingLLM *m_embLLM;
    Embeddings *m_embeddings;
};

#endif // DATABASE_H
