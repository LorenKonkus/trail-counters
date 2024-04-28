package HarvestStrava;

class SegmentMap {
	private int segmentID;
	private String segmentTrailName;
	private SegmentInfo segmentInfo;
	
	/**
	 * @return the segmentID
	 */
	protected int getSegmentID() {
		return segmentID;
	}

	/**
	 * @return the segmentTrailName
	 */
	protected String getSegmentTrailName() {
		return segmentTrailName;
	}

	/**
	 * @return the segmentInfo
	 */
	protected SegmentInfo getSegmentInfo() {
		if (segmentInfo == null) {
			segmentInfo = new SegmentInfo(segmentID);
		}
		return segmentInfo;
	}

	/**
	 * @param segmentID
	 * @param segmentFileName
	 */
	protected SegmentMap(int segmentID, String segmentTrailName) {
		super();
		this.segmentID = segmentID;
		this.segmentTrailName = segmentTrailName;
		this.segmentInfo = null;
	}
}
