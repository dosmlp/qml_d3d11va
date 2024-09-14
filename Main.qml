import QtQuick
import QtMultimedia
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FFmpegPlayer

Window {
    width: 800
    height: 600
    visible: true
    Rectangle {
        id: tophead
        width: parent.width
        height: parent.height
        color: "#FFAA00"
        ColumnLayout {
            anchors.fill: parent

            RowLayout {
                Layout.minimumHeight: 40
                Button {
                    id: mediafile
                    text: qsTr("选择文件")
                    onClicked: {
                        mediafileselect.open()
                    }
                    FileDialog {
                        id: mediafileselect
                        onAccepted: {
                            mediaplayer.source = selectedFile
                            //ppp.open(mediafileselect.currentFile)
                        }
                    }
                }

                Button {
                    id: play
                    text: "播放"
                    onClicked: {mediaplayer.play()}
                }
                Button {
                    id: pause
                    text: "暂停"
                    onClicked: {
                        mediaplayer.pause()
                    }
                }
                Button {
                    id: add5000
                    text: "+5"
                    onClicked: {
                        mediaplayer.position += 5000
                    }
                }
            }
            Rectangle {
                id: videorect
                Layout.fillHeight: true
                Layout.fillWidth: true


                MediaPlayer {
                    id: mediaplayer
                    audioOutput: AudioOutput {}
                    videoOutput: videoOutput
                    autoPlay: true
                }
                FFmpegPlayer {
                    id: ppp
                    videoSink: videoOutput.videoSink
                }

                VideoOutput {
                    id: videoOutput
                    anchors.fill: parent
                    fillMode: VideoOutput.PreserveAspectFit
                    focus: true
                    Keys.onPressed: (event)=> {
                                        console.log(event.key)
                                        if (event.key == Qt.Key_Enter) {
                                            mediaplayer.position += 5000
                                        }
                                    }
                    ShaderEffect{
                        anchors.fill: parent
                        property variant src: videoOutput
                        vertexShader: "gray.vert.qsb"
                        fragmentShader: "gray.frag.qsb"
                    }
                }


            }
        }
    }
}



