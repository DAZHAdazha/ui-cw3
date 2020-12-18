#include "videogridmodel.h"

#include <QtCore>

QVariant VideoGridModel::data(const QModelIndex &cIndex, int role) const {
    if (cIndex.isValid() && role == Qt::DisplayRole && cIndex.column() == 0) {
        auto ret = QVariant();
        int row = cIndex.row();
        if (m_data.contains(row)) {
            auto value = m_data[row];
            ret.setValue(value);
        } else {
            auto id = QSqlQueryModel::data(cIndex, role).toInt();
            auto item = createItemFrom(id, row);
            const_cast<VideoGridModel *>(this)->m_data[row] = item;
            ret.setValue(item);
        }
        return ret;
    }
    return QSqlQueryModel::data(cIndex, role);
}

VideoItem VideoGridModel::createItemFrom(int id, int row) const {
    auto path = index(row, 4).data().toString();
    auto title = index(row, 1).data().toString();
    auto dateStr = index(row, 2).data().toString();
    auto item = VideoItem(id, path, title, dateStr, index(row, 3).data().toString(),
                          index(row, 5).data().toString());
    return item;
}

void VideoGridModel::setTagId(int id) {
    m_tagId = id;
    qDebug() << "Setting tag id to " << id;
    refresh();
}

int VideoGridModel::tagId() const {
    return m_tagId;
}

void VideoGridModel::refresh() {
    QString query;
    QTextStream(&query) << "SELECT * FROM `videos` WHERE tag_id = " << m_tagId;
    setQuery(query);
}

void VideoGridModel::beginInsertItems(int start, int end) {
    m_data.clear();
    beginInsertRows(QModelIndex(), start, end);
}

void VideoGridModel::endInsertItems() {
    endInsertRows();
}

void VideoGridModel::beginRemoveItems(int start, int end) {
    m_data.clear();
    beginRemoveRows(QModelIndex(), start, end);
}

void VideoGridModel::endRemoveItems() {
    endInsertRows();
}

void VideoGridModel::changeItems(int start, int end) {
    m_data.clear();
    emit dataChanged(index(start, 0), index(end, columnCount()));
}
