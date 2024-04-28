package HarvestStrava;

class SegmentInfo {

	private boolean isValid;
	private int segment;
	private String segmentName;
	private int priorEffortCount;
	private int effortCount;
	
	
	public SegmentInfo(int segment) {
		super();
		this.isValid = false;
		this.segment = segment;
		this.segmentName = "";
		this.effortCount = -1;
		this.priorEffortCount = -1;
	}
	
	public SegmentInfo(boolean isValid, int segment, String segmentName, int effortCount) {
		super();
		this.isValid = isValid;
		this.segment = segment;
		this.segmentName = segmentName;
		this.effortCount = effortCount;
		this.priorEffortCount = -1;
	}
	
	public int getSegment() {
		return segment;
	}

	public void setSegment(int segment) {
		this.segment = segment;
	}

	public boolean isValid() {
		return isValid;
	}
	public void setValid(boolean isValid) {
		this.isValid = isValid;
	}
	public String getSegmentName() {
		return segmentName;
	}
	public void setSegmentName(String segmentName) {
		this.segmentName = segmentName;
	}
	protected int getPriorEffortCount() {
		return priorEffortCount;
	}
	public int getEffortCount() {
		return effortCount;
	}
	public void setEffortCount(int effortCount) {
		if (this.effortCount != -1)
			this.priorEffortCount = this.effortCount;
		this.effortCount = effortCount;
	}
	
}
