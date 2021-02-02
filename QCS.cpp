// wentao zhu 1/17/2021
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <string>

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmimgle/dcmimage.h"

using namespace std;

int main(int argc, char* argv[]) {
  try {
    const std::string file_path =
        "F:\\QC_data\\RI.1.3.46.423632.131000.1606838764.9.dcm";
       // "C:\\RI.1.2.246.352.81.3.3417524247.25685.18122.148.193--500mu.dcm";
    if (file_path.length() == 0) exit(EXIT_FAILURE);

    DicomImage dcm_image(file_path.c_str());

    const auto rows = dcm_image.getHeight();
    const auto cols = dcm_image.getWidth();

    DcmFileFormat file_format;
    const auto condition = file_format.loadFile(file_path.c_str());
    if (condition.bad()) {
      std::cerr << "cannot load the file. " << std::endl;
      return EXIT_FAILURE;
    }
    DcmDataset* data_set = file_format.getDataset();
    const E_TransferSyntax xfer = data_set->getOriginalXfer();
    // decompress data set if compressed
    data_set->chooseRepresentation(xfer, nullptr);
    // data_set->chooseRepresentation(EXS_LittleEndianExplicit, nullptr);
    DcmElement* element = nullptr;
    OFCondition result = data_set->findAndGetElement(DCM_PixelData, element);
    if (result.bad() || element == nullptr) return 1;
    unsigned short* pix_data;
    result = element->getUint16Array(pix_data);

    cv::Mat src_16U(rows, cols, CV_16UC1, cv::Scalar::all(0));
    unsigned short* data = nullptr;
    for (auto i = 0; i < rows; i++) {
      // could use dst2.at<unsigned short>(i, j) = ?
      data = src_16U.ptr<unsigned short>(i);
      for (auto j = 0; j < cols; j++) {
        auto temp = pix_data[i * cols + j];
        temp = temp == 65535 ? 0 : temp;
        *data++ = temp;
      }
    }
    std::vector<int> m_v;
    for (auto i = 0; i < cols; i++) {
      // std::cout << pix_data[340 * cols + i] << " ";
      m_v.emplace_back(pix_data[(rows / 2) * cols + i]);
    }
    for (auto x : m_v) {
      std::cout << x << " ";
    }
    std::cout << std::endl;

    cv::Mat src_8U(int(dcm_image.getHeight()), int(dcm_image.getWidth()),
                   CV_8UC1, (Uint8*)dcm_image.getOutputData(8));

    // cv::Mat src_gray;
    // cv::Mat detected_edges;
    // int lowThreshold = 0;
    // const int max_lowThreshold = 200;
    // const int ratio = 3;
    // const int kernel_size = 3;
    // const char* window_name = "Edge Map";
    // cv::Canny(src_8U, detected_edges, lowThreshold, lowThreshold * 2.0,
    //  kernel_size);

    cv::Mat grad_x, grad_y;
    cv::Mat abs_grad_x, abs_grad_y, dst;

    //求x方向梯度
    Sobel(src_8U, grad_x, CV_16S, 1, 0, 3, 1, 1, cv::BORDER_DEFAULT);
    convertScaleAbs(grad_x, abs_grad_x);
    // imshow("x方向soble", abs_grad_x);

    //求y方向梯度
    Sobel(src_8U, grad_y, CV_16S, 0, 1, 3, 1, 1, cv::BORDER_DEFAULT);
    convertScaleAbs(grad_y, abs_grad_y);
    // imshow("y向soble", abs_grad_y);

    //合并梯度
    addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, dst);

    // cv::Mat gray, dst, abs_dst;
    //
    ////cv::GaussianBlur(src_8U, src_8U, cv::Size(3, 3), 0, 0,
    /// cv::BORDER_DEFAULT);

    ////cvtColor(src_8U, gray, cv::COLOR_RGB2GRAY);

    //// The third parameter: the depth of the target image; the fourth
    /// parameter: / the filter aperture size; the fifth parameter: the scale
    /// factor; the
    /// sixth / parameter: the result is stored in the target image
    // Laplacian(src_8U, dst, CV_16S, 3, 1, 0, cv::BORDER_DEFAULT);
    //
    // convertScaleAbs(dst, abs_dst);

    // get the middle axis
    // first start from the center of the original graph,two gray lines

    /* get the index and find the center
     I will use template to merge later
     this row((rows / 2)-1) is the true center line.*/

    vector<int> vRows;
    abs_grad_x.row(cols / 2).copyTo(vRows);
    vector<int> vRowsIndex;

    // here, the logic could be wrong


    for (auto it = vRows.begin(); it != vRows.end(); ++it) {
      if (*it >= 10 && *it <= 100) {
        // subVRowsScale.push_back(*it);
        vRowsIndex.push_back(it - vRows.begin());
      }
    }

    for (auto x : vRowsIndex) cout << x << " ";
    cout << endl;

    int centerRowIndex = (*vRowsIndex.begin() + *(vRowsIndex.end() - 1)) / 2;

    /* vector<int> vCols;
     dst.col(rows / 2).copyTo(vCols);
     vector<int> vColsIndex;
     for (auto it = vCols.begin(); it != vCols.end(); it++) {
       if (*it == 255) vColsIndex.push_back(it - vCols.begin());
     }*/

    /*for (auto x : vColsIndex) cout << x << " ";
    cout << endl;*/
    // int centerColIndex = (*vColsIndex.begin() + *(vColsIndex.end() - 1)) / 2;

    vector<double> centerlineGrayscale;

    // cout << src_16U.row(centerColIndex) << endl;
    src_16U.row((rows / 2) - 1).copyTo(centerlineGrayscale);

    double maxElement = *std::max_element(centerlineGrayscale.begin(),
                                          centerlineGrayscale.end());

    double centerElement = centerlineGrayscale[centerRowIndex];

    double halfElement = 0.5 * (maxElement + centerElement);

    // this sub-vector is what I used to calculate flatteness
    vector<int> subVRowsScale;

    double rangeMinElement = *std::min_element(
        centerlineGrayscale.begin() + *vRowsIndex.begin(),
        centerlineGrayscale.begin() + *(vRowsIndex.end() - 1));

    double flatteness = centerElement / rangeMinElement;

    cout << "the flatteness of this graph is :" << flatteness * 100 << "%"
         << endl;

    double symmetry = 0.0;
    double gap = 9;
    for (int i = 1; i <= 10; i++) {
      symmetry = max(centerlineGrayscale[centerRowIndex + i * gap] /
                         centerlineGrayscale[centerRowIndex - i * gap],
                     symmetry);
    }

    cout << "the symmetry of this graph is: " << symmetry << endl;

    cv::imshow("16U", src_16U);
    cv::imshow("sobel", dst);

    cv::waitKey();
    cv::destroyAllWindows();
  } catch (exception& e) {
    std::cerr << "error" << e.what() << " ";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}