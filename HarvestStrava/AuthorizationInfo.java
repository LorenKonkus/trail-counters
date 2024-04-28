package HarvestStrava;

/**
 * AuthorizationInfo - context for Strava authorization
 */
class AuthorizationInfo {

	private String clientId;
	private String clientSecret;
	private String refreshToken = "";
	private String authToken = "";
	private int expiresAt = 0;
	private int expiresIn = 0;

	/**
	 * Initialize the authorization context
	 * 
	 * @param clientId
	 * @param clientSecret
	 */
	protected AuthorizationInfo() {
		super();
	}

	/**
	 * Initialize the authorization context
	 * 
	 * @param clientId
	 * @param clientSecret
	 */
	protected AuthorizationInfo(String clientId, String clientSecret) {
		super();
		this.clientId = clientId;
		this.clientSecret = clientSecret;
	}

	/**
	 * @return the refreshToken
	 */
	protected String getRefreshToken() {
		return refreshToken;
	}

	/**
	 * @param refreshToken the refreshToken to set
	 */
	protected void setRefreshToken(String refreshToken) {
		this.refreshToken = refreshToken;
	}

	/**
	 * @return the authToken
	 */
	protected String getAuthToken() {
		return authToken;
	}

	/**
	 * @param authToken the authToken to set
	 */
	protected void setAuthToken(String authToken) {
		this.authToken = authToken;
	}

	/**
	 * @return
	 */
	public int getExpiresAt() {
		return expiresAt;
	}

	/**
	 * @param expiresAt
	 */
	public void setExpiresAt(int expiresAt) {
		this.expiresAt = expiresAt;
	}

	/**
	 * @return the expiresIn
	 */
	protected int getExpiresIn() {
		return expiresIn;
	}

	/**
	 * @param expiresIn the expiresIn to set
	 */
	protected void setExpiresIn(int expiresIn) {
		this.expiresIn = expiresIn;
	}

	/**
	 * @return the clientId
	 */
	protected String getClientId() {
		return clientId;
	}

	/**
	 * @param clientId the clientId to set
	 */
	protected void setClientId(String clientId) {
		this.clientId = clientId;
	}

	/**
	 * @return the clientSecret
	 */
	protected String getClientSecret() {
		return clientSecret;
	}

	/**
	 * @param clientSecret the clientSecret to set
	 */
	protected void setClientSecret(String clientSecret) {
		this.clientSecret = clientSecret;
	}

	/**
	 * Return true if the token has expired and needs to be refreshed
	 * 
	 * @return
	 */
	protected boolean hasExpired() {
		return true;
	}

	/**
	 * Return true if there are tokens
	 * 
	 * @return
	 */
	protected boolean validTokens() {
		if (refreshToken == null || refreshToken.isEmpty()) {
			return false;
		}
		return true;
	}

}
