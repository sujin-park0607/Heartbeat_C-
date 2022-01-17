//
//  Heartbeat.cpp
//  Heartbeat
//
//  Created by Philipp Rouast on 4/06/2016.
//  Copyright © 2016 Philipp Roüast. All rights reserved.
//

#include "Heartbeat.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "opencv.hpp"

#define DEFAULT_RPPG_ALGORITHM "g"
#define DEFAULT_FACEDET_ALGORITHM "haar"
#define DEFAULT_RESCAN_FREQUENCY 1
#define DEFAULT_SAMPLING_FREQUENCY 1
#define DEFAULT_MIN_SIGNAL_SIZE 5
#define DEFAULT_MAX_SIGNAL_SIZE 5
#define DEFAULT_DOWNSAMPLE 1 // x means only every xth frame is used

#define HAAR_CLASSIFIER_PATH "haarcascade_frontalface_alt.xml"
#define DNN_PROTO_PATH "opencv/deploy.prototxt"
#define DNN_MODEL_PATH "opencv/res10_300x300_ssd_iter_140000.caffemodel"

using namespace cv;

Heartbeat::Heartbeat(int argc_, char * argv_[], bool switches_on_) {

    argc = argc_;
    argv.resize(argc);
    copy(argv_, argv_ + argc, argv.begin());
    switches_on = switches_on_;

    // map the switches to the actual
    // arguments if necessary
    if (switches_on) {

        vector<string>::iterator it1, it2;
        it1 = argv.begin();
        it2 = it1 + 1;

        while (true) {

            if (it1 == argv.end()) break;
            if (it2 == argv.end()) break;

            if ((*it1)[0] == '-')
                switch_map[*it1] = *(it2);

            it1++;
            it2++;
        }
    }
}

string Heartbeat::get_arg(int i) {

    if (i >= 0 && i < argc)
        return argv[i];

    return "";
}

string Heartbeat::get_arg(string s) {

    if (!switches_on) return "";

    if (switch_map.find(s) != switch_map.end())
        return switch_map[s];

    return "";
}

bool to_bool(string s) {
    bool result;
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    istringstream is(s);
    is >> boolalpha >> result;
    return result;
}

rPPGAlgorithm to_rppgAlgorithm(string s) {
    rPPGAlgorithm result;
    if (s == "g") result = g;
    else if (s == "pca") result = pca;
    else if (s == "xminay") result = xminay;
    else {
        std::cout << "Please specify valid rPPG algorithm (g, pca, xminay)!" << std::endl;
        exit(0);
    }
    return result;
}

faceDetAlgorithm to_faceDetAlgorithm(string s) {
    faceDetAlgorithm result;
    if (s == "haar") result = haar;
    else if (s == "deep") result = deep;
    else {
        std::cout << "Please specify valid face detection algorithm (haar, deep)!" << std::endl;
        exit(0);
    }
    return result;
}

//첫번째
int main(int argc, char * argv[]) {

    Heartbeat cmd_line(argc, argv, true);

    string input = cmd_line.get_arg("-i"); // Filepath for offline mode

    // algorithm setting
    rPPGAlgorithm rPPGAlg;
    string rppgAlgString = cmd_line.get_arg("-rppg");
    if (rppgAlgString != "") {
        rPPGAlg = to_rppgAlgorithm(rppgAlgString);
    } else {
        rPPGAlg = to_rppgAlgorithm(DEFAULT_RPPG_ALGORITHM);
    }

    cout << "Using rPPG algorithm " << rPPGAlg << "." << endl;

    // face detection algorithm setting
    faceDetAlgorithm faceDetAlg;
    string faceDetAlgString = cmd_line.get_arg("-facedet");
    if (faceDetAlgString != "") {
        faceDetAlg = to_faceDetAlgorithm(faceDetAlgString);
    } else {
        faceDetAlg = to_faceDetAlgorithm(DEFAULT_FACEDET_ALGORITHM);
    }

    cout << "Using face detection algorithm " << faceDetAlg << "." << endl;

    // rescanFrequency setting
    double rescanFrequency;
    string rescanFrequencyString = cmd_line.get_arg("-r");
    if (rescanFrequencyString != "") {
        rescanFrequency = atof(rescanFrequencyString.c_str());
    } else {
        rescanFrequency = DEFAULT_RESCAN_FREQUENCY;
    }

    // samplingFrequency setting
    double samplingFrequency;
    string samplingFrequencyString = cmd_line.get_arg("-f").c_str();
    if (samplingFrequencyString != "") {
        samplingFrequency = atof(samplingFrequencyString.c_str());
    } else {
        samplingFrequency = DEFAULT_SAMPLING_FREQUENCY;
    }

//두번째
    // max signal size setting
    int maxSignalSize;
    string maxSignalSizeString = cmd_line.get_arg("-max");
    if (maxSignalSizeString != "") {
        maxSignalSize = atof(maxSignalSizeString.c_str());
    } else {
        maxSignalSize = DEFAULT_MAX_SIGNAL_SIZE;
    }

    // min signal size setting
    int minSignalSize;
    string minSignalSizeString = cmd_line.get_arg("-min");
    if (minSignalSizeString != "") {
        minSignalSize = atof(minSignalSizeString.c_str());
    } else {
        minSignalSize = DEFAULT_MIN_SIGNAL_SIZE;
    }

    if (minSignalSize > maxSignalSize) {
        std::cout << "Max signal size must be greater or equal min signal size!" << std::endl;
        exit(0);
    }

    // Reading gui setting
    bool gui;
    string guiString = cmd_line.get_arg("-gui");
    if (guiString != "") {
        gui = to_bool(guiString);
    } else {
        gui = true;
    }

//세번재
    // Reading log setting
    bool log;
    string logString = cmd_line.get_arg("-log");
    if (logString != "") {
        log = to_bool(logString);
    } else {
        log = false;
    }

    // Reading downsample setting
    int downsample;
    string Stringdownsample = cmd_line.get_arg("-ds");
    if (downsampleString != "") {
        downsample = atof(downsampleString.c_str());
    } else {
        downsample = DEFAULT_DOWNSAMPLE;
    }

    std::ifstream test1(HAAR_CLASSIFIER_PATH);
    if (!test1) {
        std::cout << "Face classifier xml not found!" << std::endl;
        exit(0);
    }

    std::ifstream test2(DNN_PROTO_PATH);
    if (!test2) {
        std::cout << "DNN proto file not found!" << std::endl;
        exit(0);
    }

    std::ifstream test3(DNN_MODEL_PATH);
    if (!test3) {
        std::cout << "DNN model file not found!" << std::endl;
        exit(0);
    }

    bool offlineMode = input != "";

    VideoCapture cap;
    if (offlineMode) cap.open(input);
    else cap.open(0);
    if (!cap.isOpened()) {
        return -1;
    }

    std::string title = offlineMode ? "rPPG offline" : "rPPG online";
    cout << title << endl;
    cout << "Processing " << (offlineMode ? input : "live feed") << endl;

//네번재
    // Configure logfile path
    string LOG_PATH;
    if (offlineMode) {
        LOG_PATH = input.substr(0, input.find_last_of("."));
    } else {
        std::ostringstream filepath;
        filepath << "Live_ffmpeg";
        LOG_PATH = filepath.str();
    }

    // Load video information
    const int WIDTH = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    const int HEIGHT = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    const double FPS = cap.get(cv::CAP_PROP_FPS);
    const double TIME_BASE = 0.001;

    // Print video information
    cout << "SIZE: " << WIDTH << "x" << HEIGHT << endl;
    cout << "FPS: " << FPS << endl;
    cout << "TIME BASE: " << TIME_BASE << endl;

    std::ostringstream window_title;
    window_title << title << " - " << WIDTH << "x" << HEIGHT << " -rppg " << rPPGAlg << " -facedet " << faceDetAlg << " -r " << rescanFrequency << " -f " << samplingFrequency << " -min " << minSignalSize << " -max " << maxSignalSize << " -ds " << downsample;
//다섯번재
    // rppg클래스 로드하기
    RPPG rppg = RPPG();
    rppg.load(rPPGAlg, faceDetAlg,
              WIDTH, HEIGHT, TIME_BASE, downsample,
              samplingFrequency, rescanFrequency,
              minSignalSize, maxSignalSize,
              LOG_PATH, HAAR_CLASSIFIER_PATH,
              DNN_PROTO_PATH, DNN_MODEL_PATH,
              log, gui);
    //다음줄에 start algorithm입력
    cout << "START ALGORITHM" << endl;

    int i = 0;
    Mat frameRGB, frameGray;
    //반복문 시작 
    while (true) {

        // VideoCapture cap 동영상 입력을 관리하는 함수
        //재생되는 비디오의 한 frame씩 읽어서 true 반환,frame이 없을경우 false 반환 
        cap.read(frameRGB);

        //만약 frameRGB가 비어있다면 while문 탈출 
        if (frameRGB.empty())
            break;

        // bgr로 되어있는 영상을 gray로 바꾸기
        cvtColor(frameRGB, frameGray, COLOR_BGR2GRAY);
        equalizeHist(frameGray, frameGray);
        //밝기 조정을 위한 히스토그램 평활화  https://m.blog.naver.com/PostView.naver?isHttpsRedirect=true&blogId=shwotjd14&logNo=221292440961

        //시간변수 정의 
        int time;
        //만약 offlineMode라면(캠이 켜져있다면), 비디오 속성(동영상 현재 프레임수)를 받아오기
        //캠이 켜져있지 않다면, 
        // getTickCount():이벤트 시작 이후 cpu가 사용하는 클록 사이클 수를 반환
        // getTickFrequency() cpu가 걷는 클록 사이클 수를 반환 
        //두 함수를 사용하여 FPS를 계산 
        if (offlineMode) time = (int)cap.get(CAP_PROP_POS_MSEC);
        else time = (cv::getTickCount()*1000.0)/cv::getTickFrequency();

        //만약 downsample default값이 1임, i가 정수라는것을 생각햇을때 대부분은 if문이 실행되어 processFrame을 실행시킨다
        //만약 defaul값이 아니면 그건 해석이 잘 안됨 ㅜㅜ 
        //processFrame은 얼굴을 찾는 함수로 얼굴을 잡았을때 초당프레임 속도, 다른 메서드들 을 호출하게 된다. 
        if (i % downsample == 0) {
            rppg.processFrame(frameRGB, frameGray, time);
        } else {
            cout << "SKIPPING FRAME TO DOWNSAMPLE!" << endl;
        //아래 샘플로 프레임을 건너뜁니다 출력 
        }
        //gui 시각화의 유무를 선택하는것에서 True일 경우 실행 
        if (gui) {
            //위에서받아온 window_title의 이름으로 frameRGB창을 띄웁니다 
            imshow(window_title.str(), frameRGB);
            if (waitKey(30) >= 0) break;
            //30초동안 대기하고, 키 이벤트가 발생하면 해당 키 값을 반환
            //주어진 시간동안 키 이벤트 발생하지 않으면 -1반환, 키가 눌리면 while문을 나간다.
        }

        i++;
    }

    return 0;
}
