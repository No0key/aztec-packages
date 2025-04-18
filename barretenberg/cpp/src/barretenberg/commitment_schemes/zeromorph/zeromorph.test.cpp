// #include "zeromorph.hpp"
// #include "barretenberg/commitment_schemes/commitment_key.test.hpp"
// #include "barretenberg/commitment_schemes/ipa/ipa.hpp"
// #include "barretenberg/commitment_schemes/kzg/kzg.hpp"
// #include <gtest/gtest.h>

// namespace bb {

// template <class PCS> class ZeroMorphTest : public CommitmentTest<typename PCS::Curve> {
//   public:
//     using Curve = typename PCS::Curve;
//     using Fr = typename Curve::ScalarField;
//     using Polynomial = bb::Polynomial<Fr>;
//     using Commitment = typename Curve::AffineElement;
//     using GroupElement = typename Curve::Element;
//     using VerifierAccumulator = typename PCS::VerifierAccumulator;
//     using ZeroMorphProver = ZeroMorphProver_<Curve>;
//     using ZeroMorphVerifier = ZeroMorphVerifier_<Curve>;

//     using TupleOfConcatenationInputs = std::tuple<std::vector<std::vector<Polynomial>>,
//                                                   std::vector<Polynomial>,
//                                                   std::vector<Fr>,
//                                                   std::vector<std::vector<Commitment>>>;

//     /**
//      * @brief Data structure for encapsulating a set of multilinear polynomials used to test the protocol, their
//      * evaluations at the point that we want to create an evaluation proof for and
//      * their commitments. Alternatively, the polynomials and commitments can be the ones to-be-shifted, while the
//      * evaluations are for their shifted version.
//      *
//      */
//     struct PolynomialsEvaluationsCommitments {
//         std::vector<Polynomial> polynomials;
//         std::vector<Fr> evaluations;
//         std::vector<Commitment> commitments;
//     };

//     /**
//      * @brief Data structure used to test the protocol's alternative for Goblin Translator.
//      *
//      */
//     struct ConcatenationInputs {
//         std::vector<std::vector<Polynomial>> concatenation_groups;
//         std::vector<Polynomial> concatenated_polynomials;
//         std::vector<Fr> c_evaluations;
//         std::vector<std::vector<Commitment>> concatenation_groups_commitments;
//     };

//     /**
//      * @brief Evaluate Phi_k(x) = \sum_{i=0}^k x^i using the direct inefficent formula
//      *
//      */
//     Fr Phi(Fr challenge, size_t subscript)
//     {
//         size_t length = 1 << subscript;
//         auto result = Fr(0);
//         for (size_t idx = 0; idx < length; ++idx) {
//             result += challenge.pow(idx);
//         }
//         return result;
//     }

//     /**
//      * @brief Construct and verify ZeroMorph proof of batched multilinear evaluation with shifts
//      * @details The goal is to construct and verify a single batched multilinear evaluation proof for m polynomials
//      f_i
//      * and l polynomials h_i. It is assumed that the h_i are shifts of polynomials g_i (the "to-be-shifted"
//      * polynomials), which are a subset of the f_i. This is what is encountered in practice. We accomplish this using
//      * evaluations of h_i but commitments to only their unshifted counterparts g_i (which we get for "free" since
//      * commitments [g_i] are contained in the set of commitments [f_i]).
//      */
//     bool execute_zeromorph_protocol(size_t NUM_UNSHIFTED,
//                                     size_t NUM_SHIFTED,
//                                     [[maybe_unused]] size_t NUM_CONCATENATED = 0)
//     {
//         size_t N = 2;
//         size_t log_N = numeric::get_msb(N);

//         std::vector<Fr> u_challenge = this->random_evaluation_point(log_N);

//         // Construct some random multilinear polynomials f_i, their commitments and their evaluations v_i = f_i(u)
//         PolynomialsEvaluationsCommitments unshifted_input =
//             polynomials_comms_and_evaluations(u_challenge, NUM_UNSHIFTED);

//         // Construct polynomials and commitments from f_i that are to be shifted and compute their shifted
//         evaluations PolynomialsEvaluationsCommitments shifted_input =
//             to_be_shifted_polynomials_and_comms_and_shifted_evaluations(unshifted_input, u_challenge, NUM_SHIFTED);

//         bool verified = false;
//         if (NUM_CONCATENATED == 0) {
//             verified = prove_and_verify(N, unshifted_input, shifted_input, u_challenge);
//         } else {
//             verified =
//                 prove_and_verify_with_concatenation(N, unshifted_input, shifted_input, u_challenge,
//                 NUM_CONCATENATED);
//         }

//         return verified;
//     }

//     /**
//      * @brief Generate some random multilinear polynomials and compute their evaluation at the set challenge as well
//      as
//      * their commitments, returned as a tuple to be used in the subsequent protocol.
//      */
//     PolynomialsEvaluationsCommitments polynomials_comms_and_evaluations(std::vector<Fr> u_challenge,
//                                                                         size_t NUM_UNSHIFTED)
//     {
//         // Construct some random multilinear polynomials f_i and their evaluations v_i = f_i(u)
//         std::vector<Polynomial> f_polynomials; // unshifted polynomials
//         std::vector<Fr> v_evaluations;
//         std::vector<Commitment> f_commitments;
//         size_t poly_length = 1 << u_challenge.size();
//         for (size_t i = 0; i < NUM_UNSHIFTED; ++i) {
//             f_polynomials.emplace_back(Polynomial::random(poly_length, 1)); // ensure f is "shiftable"
//             v_evaluations.emplace_back(f_polynomials[i].evaluate_mle(u_challenge));
//             f_commitments.emplace_back(this->commit(f_polynomials[i]));
//         }
//         return { f_polynomials, v_evaluations, f_commitments };
//     }

//     /**
//      * @brief Generate shifts of polynomials and compute their evaluation at the
//      * set challenge as well as their commitments, returned as a tuple to be used in the subsequent protocol.
//      */
//     PolynomialsEvaluationsCommitments to_be_shifted_polynomials_and_comms_and_shifted_evaluations(
//         PolynomialsEvaluationsCommitments unshifted_inputs, std::vector<Fr> u_challenge, size_t NUM_SHIFTED)
//     {
//         std::vector<Polynomial> f_polynomials = unshifted_inputs.polynomials;
//         std::vector<Commitment> f_commitments = unshifted_inputs.commitments;

//         std::vector<Polynomial> g_polynomials; // to-be-shifted polynomials
//         std::vector<Polynomial> h_polynomials; // shifts of the to-be-shifted polynomials
//         std::vector<Fr> w_evaluations;         // shifted evaluations
//         std::vector<Commitment> g_commitments;

//         // For testing purposes, pick the first NUM_SHIFTED polynomials to be shifted
//         for (size_t i = 0; i < NUM_SHIFTED; ++i) {
//             g_polynomials.emplace_back(f_polynomials[i]);
//             h_polynomials.emplace_back(g_polynomials[i].shifted());
//             w_evaluations.emplace_back(h_polynomials[i].evaluate_mle(u_challenge));
//             g_commitments.emplace_back(f_commitments[i]);
//         }
//         return { g_polynomials, w_evaluations, g_commitments };
//     }

//     /**
//      * @brief Generate the tuple of concatenation inputs used to test Zeromorph special functionality that avoids
//      high
//      * degrees in the Goblin Translator.
//      */
//     ConcatenationInputs concatenation_inputs(std::vector<Fr> u_challenge, size_t NUM_CONCATENATED)
//     {

//         size_t concatenation_index = 2;
//         size_t N = 1 << u_challenge.size();
//         size_t MINI_CIRCUIT_N = N / concatenation_index;

//         // Polynomials "chunks" that are concatenated in the PCS
//         std::vector<std::vector<Polynomial>> concatenation_groups;

//         // Concatenated polynomials
//         std::vector<Polynomial> concatenated_polynomials;

//         // Evaluations of concatenated polynomials
//         std::vector<Fr> c_evaluations;

//         // For each polynomial to be concatenated
//         for (size_t i = 0; i < NUM_CONCATENATED; ++i) {
//             std::vector<Polynomial> concatenation_group;
//             Polynomial concatenated_polynomial(N);
//             // For each chunk
//             for (size_t j = 0; j < concatenation_index; j++) {
//                 Polynomial chunk_polynomial(N);
//                 // Fill the chunk polynomial with random values and appropriately fill the space in
//                 // concatenated_polynomial
//                 for (size_t k = 0; k < MINI_CIRCUIT_N; k++) {
//                     // Chunks should be shiftable
//                     auto tmp = Fr(0);
//                     if (k > 0) {
//                         tmp = Fr::random_element(this->engine);
//                     }
//                     chunk_polynomial.at(k) = tmp;
//                     concatenated_polynomial.at(j * MINI_CIRCUIT_N + k) = tmp;
//                 }
//                 concatenation_group.emplace_back(chunk_polynomial);
//             }
//             // Store chunks
//             concatenation_groups.emplace_back(concatenation_group);
//             // Store concatenated polynomial
//             concatenated_polynomials.emplace_back(concatenated_polynomial);
//             // Get evaluation
//             c_evaluations.emplace_back(concatenated_polynomial.evaluate_mle(u_challenge));
//         }

//         // Compute commitments of all polynomial chunks
//         std::vector<std::vector<Commitment>> concatenation_groups_commitments;
//         for (size_t i = 0; i < NUM_CONCATENATED; ++i) {
//             std::vector<Commitment> concatenation_group_commitment;
//             for (size_t j = 0; j < concatenation_index; j++) {
//                 concatenation_group_commitment.emplace_back(this->commit(concatenation_groups[i][j]));
//             }
//             concatenation_groups_commitments.emplace_back(concatenation_group_commitment);
//         }

//         return { concatenation_groups, concatenated_polynomials, c_evaluations, concatenation_groups_commitments };
//     };

//     bool prove_and_verify(size_t N,
//                           PolynomialsEvaluationsCommitments& unshifted,
//                           PolynomialsEvaluationsCommitments& shifted,
//                           std::vector<Fr> u_challenge)
//     {
//         auto prover_transcript = NativeTranscript::prover_init_empty();

//         // Execute Prover protocol
//         auto prover_opening_claim = ZeroMorphProver::prove(N,
//                                                            RefVector(unshifted.polynomials), // unshifted
//                                                            RefVector(shifted.polynomials),   // to-be shifted
//                                                            RefVector(unshifted.evaluations), // unshifted
//                                                            RefVector(shifted.evaluations),   // shifted
//                                                            u_challenge,
//                                                            this->commitment_key,
//                                                            prover_transcript);

//         PCS::compute_opening_proof(this->commitment_key, prover_opening_claim, prover_transcript);

//         auto verifier_transcript = NativeTranscript::verifier_init_empty(prover_transcript);

//         auto verifier_opening_claim = ZeroMorphVerifier::verify(N,
//                                                                 RefVector(unshifted.commitments), // unshifted
//                                                                 RefVector(shifted.commitments),   // to-be-shifted
//                                                                 RefVector(unshifted.evaluations), // unshifted
//                                                                 RefVector(shifted.evaluations),   // shifted
//                                                                 u_challenge,
//                                                                 this->vk()->get_g1_identity(),
//                                                                 verifier_transcript);
//         VerifierAccumulator result;

//         bool verified = false;
//         if constexpr (std::same_as<PCS, KZG<curve::BN254>>) {

//             result = PCS::reduce_verify(verifier_opening_claim, verifier_transcript);
//             verified = this->vk()->pairing_check(result[0], result[1]);
//         } else {
//             // Execute Verifier protocol with vk
//             verified = PCS::reduce_verify(this->vk(), verifier_opening_claim, verifier_transcript);
//         }

//         // The prover and verifier manifests should agree
//         EXPECT_EQ(prover_transcript->get_manifest(), verifier_transcript->get_manifest());
//         return verified;
//     };

//     bool prove_and_verify_with_concatenation(size_t N,
//                                              PolynomialsEvaluationsCommitments& unshifted,
//                                              PolynomialsEvaluationsCommitments& shifted,
//                                              std::vector<Fr> u_challenge,
//                                              size_t NUM_CONCATENATED)
//     {
//         ConcatenationInputs concatenation = concatenation_inputs(u_challenge, NUM_CONCATENATED);

//         auto prover_transcript = NativeTranscript::prover_init_empty();

//         // Execute Prover protocol
//         auto prover_opening_claim =
//             ZeroMorphProver::prove(N,
//                                    RefVector(unshifted.polynomials), // unshifted
//                                    RefVector(shifted.polynomials),   // to-be-shifted
//                                    RefVector(unshifted.evaluations), // unshifted
//                                    RefVector(shifted.evaluations),   // shifted
//                                    u_challenge,
//                                    this->commitment_key,
//                                    prover_transcript,
//                                    RefVector(concatenation.concatenated_polynomials),
//                                    RefVector(concatenation.c_evaluations),
//                                    to_vector_of_ref_vectors(concatenation.concatenation_groups));
//         PCS::compute_opening_proof(this->commitment_key, prover_opening_claim, prover_transcript);

//         auto verifier_transcript = NativeTranscript::verifier_init_empty(prover_transcript);

//         auto verifier_opening_claim =
//             ZeroMorphVerifier::verify(N,
//                                       RefVector(unshifted.commitments), // unshifted
//                                       RefVector(shifted.commitments),   // to-be-shifted
//                                       RefVector(unshifted.evaluations), // unshifted
//                                       RefVector(shifted.evaluations),   // shifted
//                                       u_challenge,
//                                       this->vk()->get_g1_identity(),
//                                       verifier_transcript,
//                                       to_vector_of_ref_vectors(concatenation.concatenation_groups_commitments),
//                                       RefVector(concatenation.c_evaluations));
//         VerifierAccumulator result;

//         bool verified = false;
//         if constexpr (std::same_as<PCS, KZG<curve::BN254>>) {

//             result = PCS::reduce_verify(verifier_opening_claim, verifier_transcript);
//             verified = this->vk()->pairing_check(result[0], result[1]);
//         } else {
//             // Execute Verifier protocol with vk
//             verified = PCS::reduce_verify(this->vk(), verifier_opening_claim, verifier_transcript);
//         }

//         // The prover and verifier manifests should agree
//         EXPECT_EQ(prover_transcript->get_manifest(), verifier_transcript->get_manifest());
//         return verified;
//     }
// };

// using PCSTypes = ::testing::Types<KZG<curve::BN254>, IPA<curve::Grumpkin>>;
// TYPED_TEST_SUITE(ZeroMorphTest, PCSTypes);

// /**
//  * @brief Test method for computing q_k given multilinear f
//  * @details Given f = f(X_0, ..., X_{d-1}), and (u,v) such that f(u) = v, compute q_k = q_k(X_0, ..., X_{k-1}) such
//  that
//  * the following identity holds:
//  *
//  *  f(X_0, ..., X_{d-1}) - v = \sum_{k=0}^{d-1} (X_k - u_k)q_k(X_0, ..., X_{k-1})
//  *
//  */
// TYPED_TEST(ZeroMorphTest, QuotientConstruction)
// {
//     // Define some useful type aliases
//     using Curve = typename TypeParam::Curve;
//     using ZeroMorphProver = ZeroMorphProver_<Curve>;
//     using Fr = typename Curve::ScalarField;
//     using Polynomial = bb::Polynomial<Fr>;

//     // Define size parameters
//     size_t N = 16;
//     size_t log_N = numeric::get_msb(N);

//     // Construct a random multilinear polynomial f, and (u,v) such that f(u) = v.
//     Polynomial multilinear_f = Polynomial::random(N);
//     std::vector<Fr> u_challenge = this->random_evaluation_point(log_N);
//     Fr v_evaluation = multilinear_f.evaluate_mle(u_challenge);

//     // Compute the multilinear quotients q_k = q_k(X_0, ..., X_{k-1})
//     std::vector<Polynomial> quotients = ZeroMorphProver::compute_multilinear_quotients(multilinear_f, u_challenge);

//     // Show that the q_k were properly constructed by showing that the identity holds at a random multilinear
//     challenge
//     // z, i.e. f(z) - v - \sum_{k=0}^{d-1} (z_k - u_k)q_k(z) = 0
//     std::vector<Fr> z_challenge = this->random_evaluation_point(log_N);

//     Fr result = multilinear_f.evaluate_mle(z_challenge);
//     result -= v_evaluation;
//     for (size_t k = 0; k < log_N; ++k) {
//         auto q_k_eval = Fr(0);
//         if (k == 0) {
//             // q_0 = a_0 is a constant polynomial so it's evaluation is simply its constant coefficient
//             q_k_eval = quotients[k][0];
//         } else {
//             // Construct (u_0, ..., u_{k-1})
//             auto subrange_size = static_cast<std::ptrdiff_t>(k);
//             std::vector<Fr> z_partial(z_challenge.begin(), z_challenge.begin() + subrange_size);
//             q_k_eval = quotients[k].evaluate_mle(z_partial);
//         }
//         // result = result - (z_k - u_k) * q_k(u_0, ..., u_{k-1})
//         result -= (z_challenge[k] - u_challenge[k]) * q_k_eval;
//     }

//     EXPECT_EQ(result, 0);
// }

// /**
//  * @brief Test function for constructing batched lifted degree quotient \hat{q}
//  *
//  */
// TYPED_TEST(ZeroMorphTest, BatchedLiftedDegreeQuotient)
// {
//     // Define some useful type aliases
//     using Curve = typename TypeParam::Curve;
//     using ZeroMorphProver = ZeroMorphProver_<Curve>;
//     using Fr = typename Curve::ScalarField;
//     using Polynomial = bb::Polynomial<Fr>;

//     const size_t N = 8;

//     // Define some mock q_k with deg(q_k) = 2^k - 1
//     std::vector<Fr> data_0 = { 1 };
//     std::vector<Fr> data_1 = { 2, 3 };
//     std::vector<Fr> data_2 = { 4, 5, 6, 7 };
//     Polynomial q_0(data_0);
//     Polynomial q_1(data_1);
//     Polynomial q_2(data_2);
//     std::vector<Polynomial> quotients = { q_0, q_1, q_2 };

//     auto y_challenge = Fr::random_element();

//     // Compute batched quotient \hat{q} using the prover method
//     auto batched_quotient = ZeroMorphProver::compute_batched_lifted_degree_quotient(quotients, y_challenge, N);

//     // Now explicitly define q_k_lifted = X^{N-2^k} * q_k and compute the expected batched result
//     std::array<Fr, N> data_0_lifted = { 0, 0, 0, 0, 0, 0, 0, 1 };
//     std::array<Fr, N> data_1_lifted = { 0, 0, 0, 0, 0, 0, 2, 3 };
//     std::array<Fr, N> data_2_lifted = { 0, 0, 0, 0, 4, 5, 6, 7 };
//     Polynomial q_0_lifted(data_0_lifted);
//     Polynomial q_1_lifted(data_1_lifted);
//     Polynomial q_2_lifted(data_2_lifted);

//     // Explicitly compute \hat{q}
//     auto batched_quotient_expected = Polynomial(N);
//     batched_quotient_expected += q_0_lifted;
//     batched_quotient_expected.add_scaled(q_1_lifted, y_challenge);
//     batched_quotient_expected.add_scaled(q_2_lifted, y_challenge * y_challenge);

//     EXPECT_EQ(batched_quotient, batched_quotient_expected);
// }

// /**
//  * @brief Test function for constructing partially evaluated quotient \zeta_x
//  *
//  */
// TYPED_TEST(ZeroMorphTest, PartiallyEvaluatedQuotientZeta)
// {
//     // Define some useful type aliases
//     using Curve = typename TypeParam::Curve;
//     using ZeroMorphProver = ZeroMorphProver_<Curve>;
//     using Fr = typename Curve::ScalarField;
//     using Polynomial = bb::Polynomial<Fr>;

//     const size_t N = 8;

//     // Define some mock q_k with deg(q_k) = 2^k - 1
//     std::vector<Fr> data_0 = { 1 };
//     std::vector<Fr> data_1 = { 2, 3 };
//     std::vector<Fr> data_2 = { 4, 5, 6, 7 };
//     Polynomial q_0(data_0);
//     Polynomial q_1(data_1);
//     Polynomial q_2(data_2);
//     std::vector<Polynomial> quotients = { q_0, q_1, q_2 };

//     auto y_challenge = Fr::random_element();

//     auto batched_quotient = ZeroMorphProver::compute_batched_lifted_degree_quotient(quotients, y_challenge, N);

//     auto x_challenge = Fr::random_element();

//     // Contruct zeta_x using the prover method
//     auto zeta_x = ZeroMorphProver::compute_partially_evaluated_degree_check_polynomial(
//         batched_quotient, quotients, y_challenge, x_challenge);

//     // Now construct zeta_x explicitly
//     auto zeta_x_expected = Polynomial(N);
//     zeta_x_expected += batched_quotient;
//     // q_batched - \sum_k q_k * y^k * x^{N - deg(q_k) - 1}
//     zeta_x_expected.add_scaled(q_0, -x_challenge.pow(N - 0 - 1));
//     zeta_x_expected.add_scaled(q_1, -y_challenge * x_challenge.pow(N - 1 - 1));
//     zeta_x_expected.add_scaled(q_2, -y_challenge * y_challenge * x_challenge.pow(N - 3 - 1));

//     EXPECT_EQ(zeta_x, zeta_x_expected);
// }

// /**
//  * @brief Demonstrate formulas for efficiently computing \Phi_k(x) = \sum_{i=0}^{k-1}x^i
//  * @details \Phi_k(x) = \sum_{i=0}^{k-1}x^i = (x^{2^k} - 1) / (x - 1)
//  *
//  */
// TYPED_TEST(ZeroMorphTest, PhiEvaluation)
// {
//     using Curve = typename TypeParam::Curve;
//     using Fr = typename Curve::ScalarField;
//     const size_t N = 8;
//     size_t n = numeric::get_msb(N);

//     // \Phi_n(x)
//     {
//         auto x_challenge = Fr::random_element();

//         auto efficient = (x_challenge.pow(1 << n) - 1) / (x_challenge - 1);

//         auto expected = this->Phi(x_challenge, n);

//         EXPECT_EQ(efficient, expected);
//     }

//     // \Phi_{n-k-1}(x^{2^{k + 1}}) = (x^{2^n} - 1) / (x^{2^{k + 1}} - 1)
//     {
//         auto x_challenge = Fr::random_element();

//         size_t k = 2;

//         // x^{2^{k+1}}
//         auto x_pow = x_challenge.pow(1 << (k + 1));

//         auto efficient = x_challenge.pow(1 << n) - 1; // x^N - 1
//         efficient = efficient / (x_pow - 1);          // (x^N - 1) / (x^{2^{k + 1}} - 1)

//         auto expected = this->Phi(x_pow, n - k - 1);
//         EXPECT_EQ(efficient, expected);
//     }
// }

// /**
//  * @brief Test function for constructing partially evaluated quotient Z_x
//  *
//  */
// TYPED_TEST(ZeroMorphTest, PartiallyEvaluatedQuotientZ)
// {
//     // Define some useful type aliases
//     using Curve = typename TypeParam::Curve;
//     using ZeroMorphProver = ZeroMorphProver_<Curve>;
//     using Fr = typename Curve::ScalarField;
//     using Polynomial = bb::Polynomial<Fr>;

//     const size_t N = 8;
//     size_t log_N = numeric::get_msb(N);

//     // Construct a random multilinear polynomial f, and (u,v) such that f(u) = v.
//     Polynomial multilinear_f = Polynomial::random(N);
//     Polynomial multilinear_g = Polynomial::random(N, /* starting index for shift */ 1);
//     std::vector<Fr> u_challenge = this->random_evaluation_point(log_N);
//     Fr v_evaluation = multilinear_f.evaluate_mle(u_challenge);
//     Fr w_evaluation = multilinear_g.evaluate_mle(u_challenge, /* shift = */ true);

//     auto rho = Fr::random_element();

//     // compute batched polynomial and evaluation
//     auto f_batched = multilinear_f;
//     auto g_batched = multilinear_g;
//     g_batched *= rho;
//     auto v_batched = v_evaluation + rho * w_evaluation;

//     // Define some mock q_k with deg(q_k) = 2^k - 1
//     auto q_0 = Polynomial::random(1 << 0);
//     auto q_1 = Polynomial::random(1 << 1);
//     auto q_2 = Polynomial::random(1 << 2);
//     std::vector<Polynomial> quotients = { q_0, q_1, q_2 };

//     auto x_challenge = Fr::random_element();

//     // Construct Z_x using the prover method
//     auto Z_x = ZeroMorphProver::compute_partially_evaluated_zeromorph_identity_polynomial(
//         f_batched, g_batched, quotients, v_batched, u_challenge, x_challenge);

//     // Compute Z_x directly
//     // Expand g_batched as it has a virtual 0
//     auto Z_x_expected = g_batched.full();
//     Z_x_expected.add_scaled(f_batched, x_challenge);
//     Z_x_expected.at(0) -= v_batched * x_challenge * this->Phi(x_challenge, log_N);
//     for (size_t k = 0; k < log_N; ++k) {
//         auto x_pow_2k = x_challenge.pow(1 << k);         // x^{2^k}
//         auto x_pow_2kp1 = x_challenge.pow(1 << (k + 1)); // x^{2^{k+1}}
//         // x^{2^k} * \Phi_{n-k-1}(x^{2^{k+1}}) - u_k *  \Phi_{n-k}(x^{2^k})
//         auto scalar = x_pow_2k * this->Phi(x_pow_2kp1, log_N - k - 1) - u_challenge[k] * this->Phi(x_pow_2k, log_N -
//         k); scalar *= x_challenge; scalar *= Fr(-1); Z_x_expected.add_scaled(quotients[k], scalar);
//     }

//     EXPECT_EQ(Z_x, Z_x_expected);
// }

// /**
//  * @brief Test full Prover/Verifier protocol for proving single multilinear evaluation
//  *
//  */
// TYPED_TEST(ZeroMorphTest, ProveAndVerifySingle)
// {
//     size_t num_unshifted = 1;
//     size_t num_shifted = 0;
//     auto verified = this->execute_zeromorph_protocol(num_unshifted, num_shifted);
//     EXPECT_TRUE(verified);
// }

// /**
//  * @brief Test full Prover/Verifier protocol for proving batched multilinear evaluation with shifts
//  *
//  */
// TYPED_TEST(ZeroMorphTest, ProveAndVerifyBatchedWithShifts)
// {
//     size_t num_unshifted = 3;
//     size_t num_shifted = 2;
//     auto verified = this->execute_zeromorph_protocol(num_unshifted, num_shifted);
//     EXPECT_TRUE(verified);
// }

// /**
//  * @brief Test full Prover/Verifier protocol for proving single multilinear evaluation
//  *
//  */
// TYPED_TEST(ZeroMorphTest, ProveAndVerifyWithConcatenation)
// {
//     size_t num_unshifted = 1;
//     size_t num_shifted = 0;
//     size_t num_concatenated = 3;
//     auto verified = this->execute_zeromorph_protocol(num_unshifted, num_shifted, num_concatenated);
//     EXPECT_TRUE(verified);
// }
// } // namespace bb
