// wentao zhu 1/17/2021
#if 0
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
       // "F:\\QC_data\\RI.1.3.46.423632.131000.1606838764.9.dcm";
    "C:\\RI.1.2.246.352.81.3.3417524247.25685.18122.148.193--500mu.dcm";
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

    cv::Mat binary;

    double T =
        threshold(src_8U, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    cout << "threshold : %.2f\n" << T << endl;
    cv::imshow("binary", binary);

    cv::Mat grad_x, grad_y;
    cv::Mat abs_grad_x, abs_grad_y, dst;
    Sobel(binary, grad_x, CV_16S, 1, 0, 3, 1, 1, cv::BORDER_DEFAULT);
    convertScaleAbs(grad_x, abs_grad_x);
    Sobel(binary, grad_y, CV_16S, 0, 1, 3, 1, 1, cv::BORDER_DEFAULT);
    convertScaleAbs(grad_y, abs_grad_y);
    addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, dst);
    // I don't need sobel any more.
    cv::imshow("sobel", dst);

    vector<int> vRows;
    binary.row(cols / 2).copyTo(vRows);
    vector<int> vRowsIndex;

    // here, the logic could be wrong
    // I don't know whether this is a good way to filter those strange line
    for (auto it = vRows.begin(); it != vRows.end(); ++it) {
      if ((*it == 0) && (it > vRows.begin() + 20))
      // if (*it >= 10 && *it <= 100)
      {
        // subVRowsScale.push_back(*it);
        vRowsIndex.push_back(it - vRows.begin());
      }
    }

    for (auto x : vRowsIndex) cout << x << " ";
    cout << endl;

    int centerRowIndex = (*vRowsIndex.begin() + *(vRowsIndex.end() - 1)) / 2;

    vector<int> vCols;
    binary.col(rows / 2).copyTo(vCols);
    vector<int> vColsIndex;
    for (auto it = vCols.begin(); it != vCols.end(); it++) {
      if (*it == 0) vColsIndex.push_back(it - vCols.begin());
    }

    for (auto x : vColsIndex) cout << x << " ";
    cout << endl;
    int centerColIndex = (*vColsIndex.begin() + *(vColsIndex.end() - 1)) / 2;

    vector<double> centerlineGrayscale;

    // cout << src_16U.row(centerColIndex) << endl;
    src_16U.row(centerColIndex).copyTo(centerlineGrayscale);

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

    cv::waitKey();
    cv::destroyAllWindows();
  } catch (exception& e) {
    std::cerr << "error" << e.what() << " ";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
#endif