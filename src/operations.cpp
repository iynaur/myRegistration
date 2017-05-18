#include "operations.h"

void PointCloudOperations::filter_bilateral
(
const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr & pSrcCloud,
const float & pSigmaR,
const float & pSigmaS,
pcl::PointCloud<pcl::PointXYZRGB>::Ptr &pDstCloud
)
{
#ifdef DEBUG
	std::cout << "Filtering cloud with bilateral filter...\n";
#endif

	pcl::FastBilateralFilterOMP<pcl::PointXYZRGB> bf;
	bf.setInputCloud(pSrcCloud);
	bf.setSigmaR(pSigmaR);
	bf.setSigmaS(pSigmaS);
	bf.applyFilter(*pDstCloud);

#ifdef DEBUG
	std::cout << "Cloud filtered...\n";
#endif
}

/// ---------------------------------------------------------------------------------------------

void PointCloudOperations::compute_normals
(
const pcl::PointCloud<myPointT>::ConstPtr & pCloud,
const int & pNeighbors,
//const float & pRadius,
pcl::PointCloud<myNormalT>::Ptr & pNormals
)
{
#ifdef DEBUG
	std::cout << "Estimating normals...\n";
#endif

	pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal> ne;
	pcl::search::KdTree<pcl::PointXYZRGB>::Ptr kdtree(new pcl::search::KdTree<pcl::PointXYZRGB>);
	ne.setSearchMethod(kdtree);
	ne.setKSearch(pNeighbors);
//	ne.setRadiusSearch(pRadius);
	ne.setInputCloud(pCloud);
	ne.compute(*pNormals);

#ifdef DEBUG
	std::cout << "Normals estimated (" << pNormals->size() << ") ...\n";
#endif
}

/// -----------------------------------------------------------------------------------------------


/// -----------------------------------------------------------------------------------------------

void PointCloudOperations::vg_cloud
(
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pSrcCloud,
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pDstCloud,
const float & pLeafSize
)
{

#ifdef DEBUG
	std::cout << "Voxelizing cloud(" << pSrcCloud->points.size() << ")...\n";
#endif

	pcl::VoxelGrid<pcl::PointXYZRGB> vg;
	vg.setInputCloud(pSrcCloud);
	vg.setLeafSize(pLeafSize, pLeafSize, pLeafSize);
	vg.filter(*pDstCloud);

#ifdef DEBUG
	std::cout << "Cloud voxelized(" << pDstCloud->points.size() << ")...\n";
#endif

}

/// -----------------------------------------------------------------------------------------------

/// -----------------------------------------------------------------------------------------------

void PointCloudOperations::translate_cloud
(
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pSrcCloud,
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pDstCloud,
const Eigen::Vector3f & pTranslation
)
{
	Eigen::Affine3f transform = Eigen::Affine3f::Identity();

#ifdef DEBUG
	std::cout << "Translating cloud...\n";
#endif

	transform.translation() << pTranslation[0], pTranslation[1], pTranslation[2];

#ifdef DEBUG
	std::cout << "Transformation matrix: \n";
	std::cout << transform.matrix() << "\n";
#endif

	pcl::transformPointCloud(*pSrcCloud, *pDstCloud, transform);

#ifdef DEBUG
	std::cout << "Cloud translated...\n";
#endif

}

/// -----------------------------------------------------------------------------------------------

void PointCloudOperations::rotate_cloud
(
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pSrcCloud,
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pDstCloud,
const double & pAngle,
const Eigen::Vector3f & pAxis
)
{

	float theta = pcl::deg2rad(pAngle);
	Eigen::Affine3f transform = Eigen::Affine3f::Identity();

#ifdef DEBUG
	std::cout << "Rotating cloud...\n";
#endif

	transform.translation() << 0.0, 0.0, 0.0;
	transform.rotate(Eigen::AngleAxisf(theta, pAxis));

#ifdef DEBUG
	std::cout << "Transformation matrix: \n";
	std::cout << transform.matrix() << "\n";
#endif

	pcl::transformPointCloud(*pSrcCloud, *pDstCloud, transform);

#ifdef DEBUG
	std::cout << "Cloud rotated...\n";
#endif

}

/// -----------------------------------------------------------------------------------------------

void PointCloudOperations::concatenate_clouds
(
const std::vector<pcl::PointCloud<pcl::PointXYZRGB>::Ptr> & pClouds,
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pDstCloud
)
{
	pDstCloud->clear();

	for (auto cit = pClouds.begin(); cit != pClouds.end(); ++cit)
		(*pDstCloud) = (*pDstCloud) + (**cit);
}

/// -----------------------------------------------------------------------------------------------

void PointCloudOperations::pmr_cloud
(
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pSrcCloud,
pcl::PolygonMesh & pDstMesh,
const float & pMlsSearchRadius,
const bool & pMlsPolynomialFit,
const int & pMlsPolynomialOrder,
const float & pMlsUpsamplingRadius,
const float & pMlsUpsamplingStepSize,
const float & pNeRadiusSearch,
const int & pPoissonDepth
)
{

#ifdef DEBUG
	std::cout << "Applying moving least squares filter...\n";
#endif

	pcl::MovingLeastSquares<pcl::PointXYZRGB, pcl::PointXYZRGB> mls;
	mls.setInputCloud(pSrcCloud);
	mls.setSearchRadius(pMlsSearchRadius);
	mls.setPolynomialFit(pMlsPolynomialFit);
	mls.setPolynomialOrder(pMlsPolynomialOrder);
	mls.setUpsamplingMethod(pcl::MovingLeastSquares<pcl::PointXYZRGB, pcl::PointXYZRGB>::SAMPLE_LOCAL_PLANE);
	mls.setUpsamplingRadius(pMlsUpsamplingRadius);
	mls.setUpsamplingStepSize(pMlsUpsamplingStepSize);

	pcl::PointCloud<pcl::PointXYZRGB>::Ptr smoothedCloud(new pcl::PointCloud<pcl::PointXYZRGB>());
	mls.process(*smoothedCloud);

#ifdef DEBUG
	std::cout << "Estimating normals...\n";
#endif

	pcl::NormalEstimation<pcl::PointXYZRGB, pcl::Normal> ne;
	pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGB>());
	ne.setSearchMethod(tree);
	ne.setInputCloud(smoothedCloud);
	ne.setRadiusSearch(pNeRadiusSearch);

	Eigen::Vector4f centroid;
	pcl::compute3DCentroid(*smoothedCloud, centroid);
	ne.setViewPoint(centroid[0], centroid[1], centroid[2]);

	pcl::PointCloud<pcl::Normal>::Ptr cloudNormals(new pcl::PointCloud<pcl::Normal>());
	ne.compute(*cloudNormals);

	for (auto cit = cloudNormals->begin(); cit != cloudNormals->end(); ++cit)
	{
		(*cit).normal_x *= -1;
		(*cit).normal_y *= -1;
		(*cit).normal_z *= -1;
	}

	pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloudSmoothedNormals(new pcl::PointCloud<pcl::PointXYZRGBNormal>());
	pcl::concatenateFields<pcl::PointXYZRGB, pcl::Normal, pcl::PointXYZRGBNormal>(*smoothedCloud, *cloudNormals, *cloudSmoothedNormals);

#ifdef DEBUG
	std::cout << "Reconstructing mesh...\n";
#endif

	pcl::Poisson<pcl::PointXYZRGBNormal> poisson;
	poisson.setDepth(pPoissonDepth);
	poisson.setInputCloud(cloudSmoothedNormals);
	poisson.reconstruct(pDstMesh);
}

/// -----------------------------------------------------------------------------------------------

void PointCloudOperations::icp_align_cloud
(
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pSrcCloud,
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pTgtCloud,
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pDstCloud,
const int & pMaxIterations,
const double & pEpsilon,
Eigen::Matrix4f & icpTransMatrix
)
{
	pcl::IterativeClosestPoint<pcl::PointXYZRGB, pcl::PointXYZRGB> icp;
	icp.setInputSource(pSrcCloud);
	icp.setInputTarget(pTgtCloud);
	icp.setMaximumIterations(pMaxIterations);
	icp.setTransformationEpsilon(pEpsilon);
	icp.align(*pDstCloud);
	icpTransMatrix = icp.getFinalTransformation();
#ifdef DEBUG
	if (icp.hasConverged())
	{
		std::cout << "ICP converged.\n";
		std::cout << "The score is " << icp.getFitnessScore() << "\n";
		std::cout << "Transformation matrix:\n";
		std::cout << icpTransMatrix << "\n";
	}
	else
		std::cout << "ICP did not converge\n";
#endif
}

/// -----------------------------------------------------------------------------------------------

void PointCloudOperations::icp_normals_align_cloud
(
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pSrcCloud,
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pTgtCloud,
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pDstCloud,
const int & pMaxIterations,
const double & pEpsilon
)
{
	pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr srcCloudNormals(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
	pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr tgtCloudNormals(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
	pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr dstCloudNormals(new pcl::PointCloud<pcl::PointXYZRGBNormal>);

	pcl::NormalEstimation<pcl::PointXYZRGB, pcl::PointXYZRGBNormal> ne;
	pcl::search::KdTree<pcl::PointXYZRGB>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZRGB>());
	ne.setSearchMethod(tree);
	ne.setKSearch(30);

	ne.setInputCloud(pSrcCloud);
	ne.compute(*srcCloudNormals);

	ne.setInputCloud(pTgtCloud);
	ne.compute(*tgtCloudNormals);

	pcl::IterativeClosestPointWithNormals<pcl::PointXYZRGBNormal, pcl::PointXYZRGBNormal> icp;
	icp.setInputSource(srcCloudNormals);
	icp.setInputTarget(tgtCloudNormals);
	icp.setMaximumIterations(pMaxIterations);
	icp.setTransformationEpsilon(pEpsilon);
	icp.align(*dstCloudNormals);

	pcl::copyPointCloud(*dstCloudNormals, *pDstCloud);

#ifdef DEBUG
	if (icp.hasConverged())
	{
		std::cout << "ICP converged.\n";
		std::cout << "The score is " << icp.getFitnessScore() << "\n";
		std::cout << "Transformation matrix:\n";
		std::cout << icp.getFinalTransformation() << "\n";
	}
	else
		std::cout << "ICP did not converge\n";
#endif
}

/// -----------------------------------------------------------------------------------------------

void PointCloudOperations::cut_cloud
(
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pSrcCloud,
pcl::PointCloud<pcl::PointXYZRGB>::Ptr & pDstCloud,
const int & pAmount
)
{
	std::cout << "Original point cloud: " << pSrcCloud->points.size() << " points...\n";

	double zMax = std::numeric_limits<double>::min();
	double zMin = std::numeric_limits<double>::max();

	for (auto cit = pSrcCloud->begin(); cit != pSrcCloud->end(); ++cit)
	{
		if ((*cit).z < zMin)
			zMin = (*cit).z;
		else if ((*cit).z > zMax)
			zMax = (*cit).z;
	}

	std::cout << "Maximum Z: " << zMax << "\n";
	std::cout << "Minimum Z: " << zMin << "\n";

	pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>());

	double limit = zMin + (((zMax - zMin) / 2.0) * (pAmount / 100.0));

	std::cout << "Limit: " << limit << "\n";

	for (auto cit = pSrcCloud->begin(); cit != pSrcCloud->end(); ++cit)
		if ((*cit).z < limit)
			cloud->push_back((*cit));

	pDstCloud->clear();

	for (auto cit = cloud->begin(); cit != cloud->end(); ++cit)
		pDstCloud->push_back((*cit));

	std::cout << "Filtered point cloud: " << pDstCloud->points.size() << " points...\n";
}

void PointCloudOperations::compute_cshot
(
	const pcl::PointCloud<myPointT>::Ptr & pScrCloud,
	const pcl::PointCloud<pcl::Normal>::Ptr & pNormals,
	const pcl::PointCloud<myPointT>::Ptr & kpCloud,
	//const int & pNeighbors
	const float & pRadius,
	pcl::PointCloud<pcl::SHOT1344>::Ptr & cshotDescriptor
	)
{
#ifdef DEBUG
	std::cout << "computing cshot descriptors" << std::endl;
#endif
	pcl::SHOTColorEstimationOMP<myPointT, myNormalT, pcl::SHOT1344> cshotEstimator;
	
	pcl::search::KdTree<myPointT>::Ptr kdtree(new pcl::search::KdTree<myPointT>);

	std::cout << "pScrCloud = " << pScrCloud->size() << std::endl;
	std::cout << "kpCloud = " << kpCloud->size() << std::endl;

	cshotEstimator.setInputCloud(kpCloud);
	cshotEstimator.setInputNormals(pNormals);
	cshotEstimator.setSearchMethod(kdtree);
	//cshotEstimator.setKSearch(pNeighbors);
	cshotEstimator.setSearchSurface(pScrCloud);
	cshotEstimator.setRadiusSearch(pRadius);
	cshotEstimator.compute(*cshotDescriptor);
#ifdef DEBUG
	std::cout << "CSHOT estimated (" << cshotDescriptor->size() << ")" << std::endl;
//	std::cout << *cshotDescriptor->points.begin() << std::endl;
//	cshotDescriptor->points.
#endif

}


void PointCloudOperations::compute_susan(
	const pcl::PointCloud<myPointT>::ConstPtr & pSrcCloud,
//	const pcl::PointCloud<myNormalT>::ConstPtr & pNormals,
	const float & pRadius,
	pcl::PointCloud<myPointT>::Ptr & pDstCloud

	)
{
#ifdef DEBUG
	std::cout << "computing susan keypoints" << std::endl;
#endif // DEBUG

	pcl::SUSANKeypoint<myPointT, myPointT, myNormalT> susanDetector;
	pcl::search::KdTree<myPointT>::Ptr kdtree(new pcl::search::KdTree<myPointT>);
	susanDetector.setNonMaxSupression(1);
//	susanDetector.setGeometricValidation(1);
//	susanDetector.setDistanceThreshold(1);
	susanDetector.setInputCloud(pSrcCloud);
//	susanDetector.setNormals(pNormals);
	susanDetector.setSearchMethod(kdtree);
	susanDetector.setRadiusSearch(pRadius);
	susanDetector.compute(*pDstCloud);
#ifdef DEBUG
	std::cout << "susan detected: (" << pDstCloud->size() << ")" << std::endl;
#endif // DEBUG

}


void PointCloudOperations::find_correspondences(
	const pcl::PointCloud<pcl::SHOT1344>::ConstPtr & srcDescriptors,
	const pcl::PointCloud<pcl::SHOT1344>::ConstPtr & tarDescriptors,
	const pcl::PointCloud<myPointT>::ConstPtr & srcKeypoints,
	const pcl::PointCloud<myPointT>::ConstPtr & tarKeypoints,
	const float & mathThreshold,
	pcl::CorrespondencesPtr & finalCorrespondences
	)
{
#ifdef DEBUG
	std::cout << "coarse feature matching" << std::endl;
#endif // DEBUG


	pcl::CorrespondencesPtr rawMatches(new pcl::Correspondences);
	pcl::registration::CorrespondenceEstimation<pcl::SHOT1344, pcl::SHOT1344> corresEstimator;
	corresEstimator.setInputSource(srcDescriptors);
	corresEstimator.setInputTarget(tarDescriptors);
	corresEstimator.determineCorrespondences(*rawMatches);
#ifdef DEBUG
	std::cout << "feature matched: (" << rawMatches->size() << ")" << std::endl;
#endif // DEBUG


//	pcl::CorrespondencesPtr fineMatches(new pcl::Correspondences);
	pcl::registration::CorrespondenceRejectorSampleConsensus<myPointT> corresRejector;
	corresRejector.setInputSource(srcKeypoints);
	corresRejector.setInputTarget(tarKeypoints);
	corresRejector.setInlierThreshold(mathThreshold);
	corresRejector.setMaximumIterations(10000);
	corresRejector.setRefineModel(false);
	corresRejector.setInputCorrespondences(rawMatches);
	corresRejector.getCorrespondences(*finalCorrespondences);
	std::cout << "feature remained: (" << finalCorrespondences->size() << ")" << std::endl;

}


void PointCloudOperations::get_initTransMatrix(
	const pcl::PointCloud<myPointT>::ConstPtr & srcKeypoints,
	const pcl::PointCloud<myPointT>::ConstPtr & tarKeypoints,
	const pcl::CorrespondencesPtr & correspondences,
	Eigen::Matrix4f & transMatrix)
{

	pcl::registration::TransformationEstimationSVD<myPointT, myPointT> transEstimator;
	transEstimator.estimateRigidTransformation(*srcKeypoints, *tarKeypoints, *correspondences, transMatrix);
#ifdef DEBUG
	std::cout << "initial transform matrix: " << transMatrix << std::endl;
#endif // DEBUG

}