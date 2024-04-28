package HarvestStrava;

import okhttp3.FormBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import java.time.LocalDate;
import java.time.LocalTime;
import java.time.LocalDateTime;

import java.time.format.DateTimeFormatter;

import org.json.JSONObject;

import java.io.*;

/**
 * HarvestStravaClient
 * 
 * A utility to harvest the trail usage stats for selected segments and record
 * that information in .csv files for import into spreadsheets.
 * 
 * This program is designed to run forever in the background, waking up every night
 * at 10 pm to collect the usage for each day. Strava uses oauth for authentication, 
 * and since this is designed to run in the background without user interaction a
 * refresh token must be obtained from strava and provided to this app each time it
 * is launched.
 * 
 * Data written to the overall Chapter trail .csv file:
 *     Date, Time, SegmentID, SegmentName, TrailName, TotalAttempts, AttemptsThisPeriod
 *     
 * Data written to individual trail .csv files:
 *     Date, Time, SegmentID, SegmentName, TotalAttempts, AttemptsThisPeriod
 *     
 * as Strava only provides a total "effort_count" for each trail, the field
 * AttemptsThisPeriod is not valid for the first retrieval each time this app is
 * launched, and a -1 value will be written to the .csv. It's a fairly simple matter
 * to manually patch or remove those rows as appropriate. 
 * 
 * Copyright © 2024 Loren Konkus
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
public class HarvestStravaClient {

	/**
	 * segments identifies each of the strava segments we are interested in and
	 * associates that segment with a trail name that we're familiar with. That 
	 * trail name is used as a csv file name, so choose wisely. It's fine to have
	 * more than one segment listed for the same trail - both will be recorded
	 * in the same csv file and you'll have to handle that in your spreadsheet.
	 */
	static SegmentMap[] segments = { 
			new SegmentMap(23671166, "Maybury"),
			new SegmentMap(1188713, "TreeFarm-Limeys"), 
			new SegmentMap(13410520, "WestBloomfield") };

	public static void main(String[] args) {

		AuthorizationInfo auth = new AuthorizationInfo();

		// Parse arguments
		for (int i = 0; i < args.length; i++) {
			String arg = args[i];

			switch (arg) {
			case "-token":
				i++;
				auth.setRefreshToken(args[i]);
				System.out.println("refresh token = " + auth.getRefreshToken());
				break;
			case "-clientid":
				i++;
				auth.setClientId(args[i]);
				break;
			case "-clientsecret":
				i++;
				auth.setClientSecret(args[i]);
				break;
			case "-h":
			case "--help":
				System.out.println("HarvestStrava - Fetch trail usage counts from Strava");
				System.out.println("Usage:");
				System.out.println("  -clientid      client id from Strava user profile.");
				System.out.println("  -clientsecret  client secret token from Strava user profile.");
				System.out.println("  -token         refresh token from Strava user profile.");
				System.out.println("  -version");
				System.out.println("  -help");
				return;
			case "-v":
			case "--version":
				System.out.println("Version 1.0");
				return;
			default:
				System.out.println("Unknown argument: " + arg);
			}
		}

		// Check if the authorization token is provided as a command line argument
		if (!auth.validTokens()) {
			System.out.println("Please provide the Strava refresh token as a command line argument. You can find");
			System.out.println("a current token from your strava user profile under My API Application. ");
			System.out.println("");
			System.out.println("a current token from your strava user profile under My API Application. ");
			return;
		}

		OkHttpClient httpClient = new OkHttpClient();

		while (true) {

			// Determine a data timestamp
		    LocalDateTime currentDateTime = LocalDateTime.now();
			DateTimeFormatter dateFormatter = DateTimeFormatter.ofPattern("yyyy-MM-dd");
			String dateStamp = currentDateTime.format(dateFormatter);
		    DateTimeFormatter timeFormatter = DateTimeFormatter.ofPattern("HH:mm");
			String timeStamp = currentDateTime.format(timeFormatter);

			// Refresh the token if necessary
			if (auth.hasExpired()) {
				refreshToken(httpClient, auth);
			}
			if (!auth.validTokens()) {
				System.out.println("Error refreshing tokens, exiting");
				return;
			}

			// Get the Strava stats we want from every segment in our map
			System.out.println("Gathering strava data from all watched segments at " + dateStamp+" "+timeStamp);
			for (SegmentMap segment : segments) {
				SegmentInfo segmentInfo = segment.getSegmentInfo();
				populateSegmentInfo(httpClient, auth, segmentInfo);
				System.out.println("    "+segmentInfo.getSegment() + ", " + segmentInfo.getSegmentName()
						+ ", " + segment.getSegmentTrailName() + ", " + segmentInfo.getEffortCount() + ","
						+ segmentInfo.getPriorEffortCount());
			}
			System.out.println("  Data gathered");

			System.out.println("  Writing csv files");
			for (SegmentMap segment : segments) {
				recordToChapterCSV(dateStamp, timeStamp, segment);
				recordToTrailCSV(dateStamp, timeStamp, segment);
			}
			System.out.println("  Write completed");

			System.out.println("  Sleeping until next sample");
			sleepUntilNextSample();
			System.out.println("  Awake");
		}

	}

	/**
	 * Refresh the oauth access token, which expires every few hours.
	 * 
	 * @param httpClient
	 * @param auth
	 */
	private static void refreshToken(OkHttpClient httpClient, AuthorizationInfo auth) {
		try {
			System.out.println("Trying  to refresh token");

			RequestBody formBody = new FormBody.Builder().add("client_id", auth.getClientId())
					.add("client_secret", auth.getClientSecret()).add("refresh_token", auth.getRefreshToken())
					.add("grant_type", "refresh_token").build();

			Request request = new Request.Builder().url("https://www.strava.com/api/v3/oauth/token").post(formBody)
					.build();

			Response response = httpClient.newCall(request).execute();

			// Check if the response is successful
			if (response.isSuccessful()) {
				// Parse JSON response body
				String responseBody = response.body().string();
				JSONObject jsonObject = new JSONObject(responseBody);

				System.out.println(jsonObject.toString());

				// Extract what we need from the response
				auth.setAuthToken(jsonObject.getString("access_token"));
				auth.setExpiresAt(jsonObject.getInt("expires_at"));
				auth.setExpiresIn(jsonObject.getInt("expires_in"));
				auth.setRefreshToken(jsonObject.getString("refresh_token"));
			} else {
				System.out.println(
						"Error response while fetching token: " + response.code() + " - " + response.message());
				auth.setRefreshToken("");
			}
		} catch (IOException e) {
			System.out.println("Exception fetching token: " + e.getMessage());
			auth.setRefreshToken("");
		}
	}

	/**
	 * Fetch current usage counts for this segment from Strava
	 * 
	 * @param httpClient
	 * @param auth
	 * @param segment
	 */
	private static void populateSegmentInfo(OkHttpClient httpClient, AuthorizationInfo auth, SegmentInfo segment) {

		// Create a GET request with authorization header
		Request request = new Request.Builder().url("https://www.strava.com/api/v3/segments/" + segment.getSegment())
				.header("Authorization", "Bearer " + auth.getAuthToken()).build();

		try {
			// Execute the request
			Response response = httpClient.newCall(request).execute();

			// Check if the response is successful
			if (response.isSuccessful()) {
				// Parse JSON response body
				String responseBody = response.body().string();
				JSONObject jsonObject = new JSONObject(responseBody);

				// System.out.println(jsonObject.toString());

				// Extract what we need from the response
				segment.setSegmentName(filterName(jsonObject.getString("name")));
				segment.setEffortCount(jsonObject.getInt("effort_count"));
				segment.setValid(true);
			} else {
				System.out.println("Error response while fetching data for segment " + segment.getSegment() + ": "
						+ response.code() + " - " + response.message());
				segment.setValid(false);
			}
		} catch (IOException e) {
			System.out.println("Exception fetching data for segment " + segment.getSegment() + ": " + e.getMessage());
			segment.setValid(false);
		}
	}

	/**
	 * Record this usage data in the overall chapter trail csv file, which includes 
	 * the usage states for every segment we're watching
	 * 
	 * @param dataTimeStamp
	 * @param segment
	 */
	public static void recordToChapterCSV(String dateStamp, String timeStamp, SegmentMap segment) {
		String fileName = "ChapterTrails.csv";
		SegmentInfo segmentInfo = segment.getSegmentInfo();
		int effortCount = segmentInfo.getEffortCount();
		int priorEffortCount = segmentInfo.getPriorEffortCount();

		try {
			// Open csv file in append mode
			FileWriter fileWriter = new FileWriter(fileName, true);
			BufferedWriter bufferedWriter = new BufferedWriter(fileWriter);
			PrintWriter printWriter = new PrintWriter(bufferedWriter);

			// If this is a new file, print the csv header line
			File file = new File(fileName);
			if (file.length() == 0)
				printWriter.println("Date, Time, SegmentID, SegmentName, TrailName, TotalAttempts, AttemptsThisPeriod");

			// record strava data
			printWriter.println(dateStamp + ", " + timeStamp + ", " + segmentInfo.getSegment() + ", " + segmentInfo.getSegmentName()
					+ ", " + segment.getSegmentTrailName() + ", " + effortCount + ", "
					+ attemptsThisPeriod(effortCount, priorEffortCount));

			// Close the file
			printWriter.close();
			bufferedWriter.close();
			fileWriter.close();
		} catch (IOException e) {
			System.err.println("Error writing file " + fileName + ": " + e.getMessage());
		}
	}

	/**
	 * Record this usage data to the trail-specific csv file. 
	 * 
	 * @param dataTimeStamp
	 * @param segment
	 */
	public static void recordToTrailCSV(String dateStamp, String timeStamp, SegmentMap segment) {
		String fileName = segment.getSegmentTrailName() + ".csv";
		SegmentInfo segmentInfo = segment.getSegmentInfo();
		int effortCount = segmentInfo.getEffortCount();
		int priorEffortCount = segmentInfo.getPriorEffortCount();

		try {
			// Open csv file in append mode
			FileWriter fileWriter = new FileWriter(fileName, true);
			BufferedWriter bufferedWriter = new BufferedWriter(fileWriter);
			PrintWriter printWriter = new PrintWriter(bufferedWriter);

			// If this is a new file, print the csv header line
			File file = new File(fileName);
			if (file.length() == 0)
				printWriter.println("Date, Time, SegmentID, SegmentName, TotalAttempts, AttemptsThisPeriod");

			// record strava data
			printWriter.println(dateStamp + ", " + timeStamp + ", " + segmentInfo.getSegment() + ", " + segmentInfo.getSegmentName()+ ", " + effortCount + ", "
					+ attemptsThisPeriod(effortCount, priorEffortCount));

			// Close the file
			printWriter.close();
			bufferedWriter.close();
			fileWriter.close();
		} catch (IOException e) {
			System.err.println("Error writing file " + fileName + ": " + e.getMessage());
		}
	}

	/**
	 * Sometimes a segment name might have characters embedded within it that can cause
	 * improper parsing when importing a csv into a spreadsheet. Make sure that doesn't
	 * happen.
	 * 
	 * @param name
	 * @return
	 */
	private static String filterName(String name) {
        String result = name.replaceAll(",", "");
        return result;
	}
	
	/**
	 * Find the attempts during this period. Strava data can be a bit messy, and go
	 * negative if public rides are taken private - ignore that noise to simplify
	 * spreadsheets
	 * 
	 * @param effortCount
	 * @param priorEffortCount
	 * @return
	 */
	private static int attemptsThisPeriod(int effortCount, int priorEffortCount) {
		int attemptsThisPeriod = -1;
		if (priorEffortCount != -1)
			attemptsThisPeriod = Math.max(effortCount - priorEffortCount, 0);
		return attemptsThisPeriod;
	}

	/**
	 * Sleep until 10 pm tomorrow, when we'll collect the next usage sample
	 */
	private static void sleepUntilNextSample() {

		// Calculate the target date and time for 10:00 PM tomorrow
		LocalDateTime currentDateTime = LocalDateTime.now();
		LocalDate tomorrow = LocalDate.now().plusDays(1);
		LocalTime targetTime = LocalTime.of(22, 0); // 10:00 PM
		LocalDateTime targetDateTime = LocalDateTime.of(tomorrow, targetTime);

		// Calculate the duration until then
		long durationUntilTarget = java.time.Duration.between(currentDateTime, targetDateTime).toMillis();
		//durationUntilTarget = 1000 * 60 * 60;
		
		// Sleep the program
		try {
			Thread.sleep(durationUntilTarget);
		} catch (InterruptedException e) {
			// Handle interruption
			e.printStackTrace();
		}
	}

}
