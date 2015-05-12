package edu.uc.rphash;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Random;

import edu.uc.rphash.Readers.RPHashObject;
import edu.uc.rphash.Readers.RPVector;
import edu.uc.rphash.Readers.SimpleArrayReader;
import edu.uc.rphash.decoders.Decoder;
import edu.uc.rphash.decoders.E8;
import edu.uc.rphash.decoders.Leech;
import edu.uc.rphash.decoders.MultiDecoder;
import edu.uc.rphash.frequentItemSet.ItemSet;
import edu.uc.rphash.frequentItemSet.KHHCountMinSketch;
import edu.uc.rphash.frequentItemSet.StickyWrapper;
import edu.uc.rphash.lsh.LSH;
import edu.uc.rphash.projections.DBFriendlyProjection;
import edu.uc.rphash.projections.Projector;
import edu.uc.rphash.standardhash.HashAlgorithm;
import edu.uc.rphash.standardhash.MurmurHash;
import edu.uc.rphash.standardhash.NoHash;
import edu.uc.rphash.tests.Agglomerative;
import edu.uc.rphash.tests.GenerateData;
import edu.uc.rphash.tests.Kmeans;
import edu.uc.rphash.tests.StatTests;
import edu.uc.rphash.tests.TestUtil;

/**
 * This is the correlated multi projections approach. In this RPHash variation
 * we try to incorporate the advantage of multiple random projections in order
 * to combat increasing cluster error rates as the deviation between projected
 * and full data increases. The main idea is similar to the referential RPHash,
 * however the set union is projection id dependent. This will be done in a
 * simplified bitmask addition to the hash code in lieu of an array of sets data
 * structures.
 * 
 * @author lee
 *
 */
public class RPHashMultiProj implements Clusterer {
	float variance;

	public RPHashObject map() {


		

		Iterator<float[]> vecs = so.getVectorIterator();
		if (!vecs.hasNext())
			return so;

		long hash;
		int probes = so.getNumProjections();
		int k = (int) (so.getk() * probes);
		Random r = new Random(so.getRandomSeed());
		
		//initialize our counter
		ItemSet<Long> is = new KHHCountMinSketch<Long>(k);
		
		// create our LSH Device
		LSH[] lshfuncs = new LSH[probes];
		Decoder dec = so.getDecoderType();
		if(dec==null){
			Decoder inner = new Leech(variance);
			dec = new MultiDecoder( so.getInnerDecoderMultiplier()*inner.getDimensionality(), inner);
		}
		HashAlgorithm hal = new MurmurHash(so.getHashmod());
		
		//create projection matrices add to LSH Device
		for (int i = 0; i < probes; i++) {
			Projector p = new DBFriendlyProjection(so.getdim(),
					dec.getDimensionality(), r.nextLong());
			lshfuncs[i] = new LSH(dec, p, hal);
		}

		// add to frequent itemset the hashed Decoded randomly projected vector
		while (vecs.hasNext()) {
			float[] vec = vecs.next();
			for (int i = 0; i < probes; i++) {
				hash = lshfuncs[i].lshHash(vec);
				// vec.id.add(hash);
				is.add(hash);
			}
		}
		so.setPreviousTopID(is.getTop());

//		for (Long l : is.getCounts())
//			System.out.printf(" %d,", l);
//		System.out.printf("\n,");
		return so;

	}

	/*
	 * This is the second phase after the top ids have been in the reduce phase
	 * aggregated
	 */
	public RPHashObject reduce() {
		
		Iterator<float[]> vecs = so.getVectorIterator();
		if (!vecs.hasNext())
			return so;

		int blurValue = so.getNumBlur();
		int probes = so.getNumProjections();
		Random r = new Random(so.getRandomSeed());

		long hash[];
		// make a set of k default centroid objects
		ArrayList<Centroid> centroids = new ArrayList<Centroid>();
		for (long id : so.getPreviousTopID())
			centroids.add(new Centroid(so.getdim(), id));
		
		
		//create same LSH Device as before
		LSH[] lshfuncs = new LSH[probes];
		Decoder dec = so.getDecoderType();
		if(dec==null){
			Decoder inner = new Leech(variance);
			dec = new MultiDecoder( so.getInnerDecoderMultiplier()*inner.getDimensionality(), inner);
		}
		HashAlgorithm hal = new MurmurHash(so.getHashmod());
		
		//create same projection matrices as before
		for (int i = 0; i < probes; i++) {
			Projector p = new DBFriendlyProjection(so.getdim(),
					dec.getDimensionality(), r.nextLong());
			lshfuncs[i] = new LSH(dec, p, hal);
		}


		if (blurValue == 0)
			blurValue = (int) Math.log(so.getdim() / dec.getDimensionality());
		
		while (vecs.hasNext()) {
			float[] vec = vecs.next();
			// iterate over the multiple projections
			for (LSH lshfunc : lshfuncs) {
				hash = lshfunc.lshHashRadius(vec, blurValue);
				// iterate over the blurred vectors
				for (Centroid cent : centroids) {
					for (long h : hash) {
						if (cent.ids.contains(h)) {
							cent.updateVec(vec);
							break;
						}
					}
				}
			}
		}
		for (Centroid cent : centroids)
			so.addCentroid(cent.centroid());

		return so;
	}

	private List<float[]> centroids = null;
	private RPHashObject so;

	public RPHashMultiProj(List<float[]> data, int k) {
		variance = StatTests.varianceSample(data, .01f);
		so = new SimpleArrayReader(data, k, 0, 2, 3);
	}

	public RPHashMultiProj(List<float[]> data, int k, int numProjections) {
		variance = StatTests.varianceSample(data, .01f);
		so = new SimpleArrayReader(data, k, 0, 2, numProjections);
	}

	public RPHashMultiProj(List<float[]> data, int k, int decmult,
			int numProjections) {
		variance = StatTests.varianceSample(data, .01f);
		so = new SimpleArrayReader(data, k, 0, decmult, numProjections);
	}

	public RPHashMultiProj(RPHashObject so) {
		this.so = so;
	}

	public List<float[]> getCentroids(RPHashObject so) {
		this.so = so;
		if (centroids == null)
			run();
		return centroids;
	}

	@Override
	public List<float[]> getCentroids() {

		if (centroids == null)
			run();
		return new Kmeans(so.getk(),centroids).getCentroids();
	}

	private void run() {

		map();
		reduce();
		centroids = new Kmeans(so.getk(), so.getCentroids()).getCentroids();
	}

	public static void main(String[] args) {
		int k = 10;
		int d = 1000;
		int n = 10000;
		float var = .3f;
		for (float f = var; f < 1.0; f += .01f) {
			for (int i = 0; i < 5; i++) {
				GenerateData gen = new GenerateData(k, n / k, d, f, true, 1f);
				RPHashMultiProj rphit = new RPHashMultiProj(gen.data(), k);

				long startTime = System.nanoTime();
				rphit.getCentroids();
				long duration = (System.nanoTime() - startTime);
				List<float[]> aligned = TestUtil.alignCentroids(
						rphit.getCentroids(), gen.medoids());
				System.out.println(f + ":" + StatTests.PR(aligned, gen) + ":"
						+ duration / 1000000000f);
				System.gc();
			}
		}
	}

	@Override
	public RPHashObject getParam() {
		return so;
	}

}
