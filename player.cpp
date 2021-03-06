#include <QAudioProbe>
#include <QMediaMetaData>
#include <QMediaPlaylist>
#include <QMediaService>
#include <QVideoProbe>
#include <QtWidgets>
using namespace std;

#include "absolutesetstyle.h"
#include "library/library.h"
#include "player.h"
#include "playlist/playlistdelegate.h"

const QFont kFontEczer("Eczar-Regular", 12, QFont::Normal);
const QFont kFontRoboto("RobotoCondensed-Regular", 12, QFont::Normal);

Player::Player(QWidget *parent, Library *_library)
    : QWidget(parent), videoWidget(0), slider(0), colorDialog(0), library(_library) {
    player = new QMediaPlayer(this);
    playlist = new QMediaPlaylist();
    player->setPlaylist(playlist);
    player->setNotifyInterval(20);
    setGeometry(500, 300, 1000, 600);
    setMinimumSize(650, 450);
    videoWidget = new VideoWidget(this);
    player->setVideoOutput(videoWidget);
    playlistModel = new PlaylistModel(this);
    playlistModel->setPlaylist(playlist);
    playlistView = new QListView(this);
    playlistView->setItemDelegate(new PlaylistDelegate);
    playlistView->setModel(playlistModel);
    playlistView->setCurrentIndex(playlistModel->index(playlist->currentIndex(), 0));
    connect(playlistView, SIGNAL(activated(QModelIndex)), this, SLOT(jump(QModelIndex)));
    slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(0, player->duration());
    slider->setStyle(new AbsoluteSetStyle(slider->style()));
    connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(seek(int)));
    labelDuration = new QLabel(this);
    openButton = new QPushButton(this);
    openButton->setIcon(QIcon(":/add.png"));
    openButton->setIconSize(QSize(25, 25));
    connect(openButton, SIGNAL(clicked()), this, SLOT(open()));
    removeButton = new QPushButton(this);
    removeButton->setIcon(QIcon(":/delete.png"));
    removeButton->setIconSize(QSize(25, 25));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeSelected()));
    libraryButton = new QPushButton("Library");
    libraryButton->setFont(kFontRoboto);
    connect(libraryButton, SIGNAL(clicked()), this, SLOT(showLibrary()));
    fullScreenButton = new QPushButton(this);
    fullScreenButton->setIcon(QIcon(":/fullscreen.png"));
    fullScreenButton->setIconSize(QSize(25, 25));
    fullScreenButton->setCheckable(true);
    initPlayerSignals();
    initLayout();
}

Player::~Player() {}

void Player::initPlayerSignals() {
    connect(player, SIGNAL(durationChanged(qint64)), SLOT(durationChanged(qint64)));
    connect(player, SIGNAL(positionChanged(qint64)), SLOT(positionChanged(qint64)));
    connect(player, &QMediaPlayer::currentMediaChanged, this, &Player::currentMediaChanged);
    connect(playlist, SIGNAL(currentIndexChanged(int)), SLOT(playlistPositionChanged(int)));
    connect(player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this,
            SLOT(statusChanged(QMediaPlayer::MediaStatus)));
    connect(player, SIGNAL(bufferStatusChanged(int)), this, SLOT(bufferingProgress(int)));
    connect(player, SIGNAL(videoAvailableChanged(bool)), this, SLOT(videoAvailableChanged(bool)));
    connect(player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(displayErrorMessage()));
    connect(player, &QMediaPlayer::stateChanged, this, &Player::stateChanged);
}

PlayerControls *Player::initControls() {
    PlayerControls *controls = new PlayerControls(this);
    controls->setState(player->state());
    controls->setVolume(player->volume());
    controls->setMuted(controls->isMuted());

    connect(controls, SIGNAL(play()), player, SLOT(play()));
    connect(controls, SIGNAL(pause()), player, SLOT(pause()));
    connect(controls, SIGNAL(stop()), player, SLOT(stop()));
    connect(controls, SIGNAL(next()), playlist, SLOT(next()));
    connect(controls, SIGNAL(previous()), this, SLOT(previousClicked()));
    connect(controls, SIGNAL(changeVolume(int)), player, SLOT(setVolume(int)));
    connect(controls, SIGNAL(changeMuting(bool)), player, SLOT(setMuted(bool)));
    connect(controls, SIGNAL(changeRate(qreal)), player, SLOT(setPlaybackRate(qreal)));
    connect(controls, SIGNAL(forward()), this, SLOT(goForward()));
    connect(controls, SIGNAL(back()), this, SLOT(goBack()));
    connect(controls, SIGNAL(stop()), videoWidget, SLOT(update()));
    connect(player, SIGNAL(stateChanged(QMediaPlayer::State)), controls,
            SLOT(setState(QMediaPlayer::State)));
    connect(player, SIGNAL(volumeChanged(int)), controls, SLOT(setVolume(int)));
    connect(player, SIGNAL(mutedChanged(bool)), controls, SLOT(setMuted(bool)));
    return controls;
}

void Player::initLayout() {
    QWidget *listWindow = new QWidget();
    QWidget *inferiorWindow = new QWidget();
    QWidget *upperWindow = new QWidget();
    QPalette pal(inferiorWindow->palette());
    pal.setColor(QPalette::Background, Qt::white);
    inferiorWindow->setAutoFillBackground(true);
    inferiorWindow->setPalette(pal);
    QBoxLayout *listLayout = new QVBoxLayout;
    QBoxLayout *inferiorLayout = new QHBoxLayout;
    QBoxLayout *upperLayout = new QHBoxLayout;
    tag = new QPushButton("test");
    tag->setFont(kFontRoboto);
    tag->setIcon(QIcon(":/tag.png"));
    tag->setIconSize(QSize(25, 25));
    videoAmount = new QLabel(this);
    updateVideoCount();
    upperLayout->addWidget(tag);
    upperWindow->setLayout(upperLayout);
    inferiorLayout->addWidget(videoAmount);
    inferiorLayout->addWidget(openButton);
    inferiorLayout->addWidget(removeButton);
    inferiorWindow->setLayout(inferiorLayout);
    listLayout->addWidget(upperWindow, 1);
    listLayout->addWidget(playlistView, 8);
    listLayout->addWidget(inferiorWindow, 1);
    listLayout->setSpacing(0);
    listWindow->setLayout(listLayout);
    QBoxLayout *displayLayout = new QHBoxLayout;
    displayLayout->addWidget(videoWidget, 3);
    displayLayout->addWidget(listWindow, 1);
    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->addWidget(initControls());
    controlLayout->addStretch(1);
    controlLayout->addWidget(libraryButton);
    controlLayout->addWidget(fullScreenButton);
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(slider);
    hLayout->addWidget(labelDuration);
    QBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(displayLayout, 2);
    layout->addLayout(hLayout);
    layout->addLayout(controlLayout);
    setLayout(layout);
}

bool Player::isPlayerAvailable() const {
    return player->isAvailable();
}

void Player::open() {
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Files"));
    fileDialog.setMimeTypeFilters(player->supportedMimeTypes());
    if (fileDialog.exec() == QDialog::Accepted) addToPlaylist(fileDialog.selectedUrls());
    updateVideoCount();
}

void Player::updateVideoCount() {
    videoAmount->setFont(kFontEczer);
    int videoAmountNumber = playlistView->model()->rowCount();
    QString videoAmountStr;
    QTextStream(&videoAmountStr) << "Total " << videoAmountNumber << " videos";
    videoAmount->setText(videoAmountStr);
}

void Player::addToPlaylist(const QList<QUrl> urls) {
    foreach (const QUrl &url, urls) { playlist->addMedia(url); }
    updateVideoCount();
}

void Player::goForward() {
    player->setPosition(player->position() + 10000);
}

void Player::goBack() {
    player->setPosition(player->position() - 10000);
}

void Player::removeSelected() {
    auto current = playlistView->currentIndex();
    if (current.isValid()) {
        playlist->removeMedia(current.row());
    }
    updateVideoCount();
}

void Player::durationChanged(qint64 duration) {
    this->duration = duration;
    slider->setMaximum(duration);
}

void Player::positionChanged(qint64 progress) {
    if (!slider->isSliderDown()) {
        slider->setValue(progress);
    }
    updateDurationInfo(progress);
}

void Player::currentMediaChanged(const QMediaContent &media) {
    if (!media.isNull()) {
        auto url = media.canonicalRequest().url().path();
        setTrackInfo(QFileInfo(url).fileName());
    }
}

void Player::previousClicked() {
    if (player->position() <= 5)
        playlist->previous();
    else
        player->setPosition(0);
}

void Player::jump(const QModelIndex &index) {
    if (index.isValid()) {
        jumpToRow(index.row());
    }
}

void Player::playlistPositionChanged(int currentItem) {
    playlistView->setCurrentIndex(playlistModel->index(currentItem, 0));
}

void Player::seek(int seconds) {
    player->setPosition(seconds);
}

void Player::statusChanged(QMediaPlayer::MediaStatus status) {
    handleCursor(status);
    switch (status) {
        case QMediaPlayer::UnknownMediaStatus:
        case QMediaPlayer::NoMedia:
        case QMediaPlayer::LoadedMedia:
        case QMediaPlayer::BufferingMedia:
        case QMediaPlayer::BufferedMedia:
            setStatusInfo(QString());
            break;
        case QMediaPlayer::LoadingMedia:
            setStatusInfo(tr("Loading..."));
            break;
        case QMediaPlayer::StalledMedia:
            setStatusInfo(tr("Media Stalled"));
            break;
        case QMediaPlayer::EndOfMedia:
            QApplication::alert(this);
            break;
        case QMediaPlayer::InvalidMedia:
            displayErrorMessage();
            break;
    }
}

void Player::stateChanged(QMediaPlayer::State state) {
    qInfo()<<state;
}

void Player::handleCursor(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::LoadingMedia || status == QMediaPlayer::BufferingMedia ||
        status == QMediaPlayer::StalledMedia)
        setCursor(QCursor(Qt::BusyCursor));
    else
        unsetCursor();
}

void Player::bufferingProgress(int progress) {
    setStatusInfo(tr("Buffering %4%").arg(progress));
}

void Player::videoAvailableChanged(bool available) {
    if (!available) {
        disconnect(fullScreenButton, SIGNAL(clicked(bool)), videoWidget, SLOT(setFullScreen(bool)));
        disconnect(videoWidget, SIGNAL(fullScreenChanged(bool)), fullScreenButton,
                   SLOT(setChecked(bool)));
        videoWidget->setFullScreen(false);
    } else {
        connect(fullScreenButton, SIGNAL(clicked(bool)), videoWidget, SLOT(setFullScreen(bool)));
        connect(videoWidget, SIGNAL(fullScreenChanged(bool)), fullScreenButton,
                SLOT(setChecked(bool)));

        if (fullScreenButton->isChecked()) videoWidget->setFullScreen(true);
    }
}

void Player::setTrackInfo(const QString &info) {
    trackInfo = info;
    updateWindowTitle();
}

void Player::setStatusInfo(const QString &info) {
    statusInfo = info;
    updateWindowTitle();
}

void Player::updateWindowTitle() {
    if (!statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2 - Tomeo").arg(trackInfo).arg(statusInfo));
    else
        setWindowTitle(QString("%1 - Tomeo").arg(trackInfo));
}

void Player::displayErrorMessage() {
    setStatusInfo(player->errorString());
}

void Player::updateDurationInfo(qint64 currentInfo) {
    QString tStr;
    if (currentInfo || duration) {
        QTime currentTime((currentInfo / 3600000) % 60, (currentInfo / 60000) % 60,
                          currentInfo / 1000 % 60, (currentInfo) % 1000);
        QTime totalTime((duration / 3600000) % 60, (duration / 60000) % 60, duration / 1000 % 60,
                        (duration) % 1000);
        QString format = "mm:ss";
        if (duration > 3600) format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    labelDuration->setFont(kFontRoboto);
    labelDuration->setText(tStr);
}

void Player::showLibrary() {
    if (library->isVisible()) {
        library->hide();
    } else {
        library->show();
    }
}

void Player::clearPlaylist() {
    playlist->clear();
    updateVideoCount();
}

void Player::jumpToRow(int row) {
    playlist->setCurrentIndex(row);
    player->play();
}

void Player::closeEvent(QCloseEvent *event) {
    player->stop();
    QWidget::closeEvent(event);
}

void Player::setTagName(const QString &name) {
    tag->setText(name);
}
