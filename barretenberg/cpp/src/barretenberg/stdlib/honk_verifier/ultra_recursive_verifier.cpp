#include "barretenberg/stdlib/honk_verifier/ultra_recursive_verifier.hpp"
#include "barretenberg/commitment_schemes/shplonk/shplemini.hpp"
#include "barretenberg/numeric/bitop/get_msb.hpp"
#include "barretenberg/plonk_honk_shared/library/grand_product_delta.hpp"
#include "barretenberg/stdlib/primitives/public_input_component/public_input_component.hpp"
#include "barretenberg/transcript/transcript.hpp"

namespace bb::stdlib::recursion::honk {

template <typename Flavor>
UltraRecursiveVerifier_<Flavor>::UltraRecursiveVerifier_(
    Builder* builder, const std::shared_ptr<NativeVerificationKey>& native_verifier_key)
    : key(std::make_shared<VerificationKey>(builder, native_verifier_key))
    , builder(builder)
{}

template <typename Flavor>
UltraRecursiveVerifier_<Flavor>::UltraRecursiveVerifier_(Builder* builder, const std::shared_ptr<VerificationKey>& vkey)
    : key(vkey)
    , builder(builder)
{}

/**
 * @brief This function constructs a recursive verifier circuit for a native Ultra Honk proof of a given flavor.
 * @return Output aggregation object
 */
template <typename Flavor>
UltraRecursiveVerifier_<Flavor>::Output UltraRecursiveVerifier_<Flavor>::verify_proof(const HonkProof& proof,
                                                                                      AggregationObject agg_obj)
{
    StdlibProof<Builder> stdlib_proof = bb::convert_native_proof_to_stdlib(builder, proof);
    return verify_proof(stdlib_proof, agg_obj);
}

/**
 * @brief This function constructs a recursive verifier circuit for a native Ultra Honk proof of a given flavor.
 * @return Output aggregation object
 */
template <typename Flavor>
UltraRecursiveVerifier_<Flavor>::Output UltraRecursiveVerifier_<Flavor>::verify_proof(const StdlibProof<Builder>& proof,
                                                                                      AggregationObject agg_obj)
{
    using Sumcheck = ::bb::SumcheckVerifier<Flavor>;
    using PCS = typename Flavor::PCS;
    using Curve = typename Flavor::Curve;
    using Shplemini = ::bb::ShpleminiVerifier_<Curve, Flavor::USE_PADDING>;
    using VerifierCommitments = typename Flavor::VerifierCommitments;
    using Transcript = typename Flavor::Transcript;
    using ClaimBatcher = ClaimBatcher_<Curve>;
    using ClaimBatch = ClaimBatcher::Batch;
    using PublicAggState = PublicInputComponent<aggregation_state<Builder>>;

    Output output;
    StdlibProof<Builder> honk_proof;
    if constexpr (HasIPAAccumulator<Flavor>) {
        const size_t HONK_PROOF_LENGTH = Flavor::NativeFlavor::PROOF_LENGTH_WITHOUT_PUB_INPUTS - IPA_PROOF_LENGTH;
        const size_t num_public_inputs = static_cast<uint32_t>(key->num_public_inputs.get_value());
        // The extra calculation is for the IPA proof length.
        // TODO(https://github.com/AztecProtocol/barretenberg/issues/1182): Handle in ProofSurgeon.
        ASSERT(proof.size() == HONK_PROOF_LENGTH + IPA_PROOF_LENGTH + num_public_inputs);
        // split out the ipa proof
        const std::ptrdiff_t honk_proof_with_pub_inputs_length =
            static_cast<std::ptrdiff_t>(HONK_PROOF_LENGTH + num_public_inputs);
        output.ipa_proof = StdlibProof<Builder>(proof.begin() + honk_proof_with_pub_inputs_length, proof.end());
        honk_proof = StdlibProof<Builder>(proof.begin(), proof.end() + honk_proof_with_pub_inputs_length);
    } else {
        honk_proof = proof;
    }
    transcript = std::make_shared<Transcript>(honk_proof);
    auto verification_key = std::make_shared<RecursiveDeciderVK>(builder, key);
    OinkVerifier oink_verifier{ builder, verification_key, transcript };
    oink_verifier.verify();

    VerifierCommitments commitments{ key, verification_key->witness_commitments };

    auto gate_challenges = std::vector<FF>(CONST_PROOF_SIZE_LOG_N);
    for (size_t idx = 0; idx < CONST_PROOF_SIZE_LOG_N; idx++) {
        gate_challenges[idx] = transcript->template get_challenge<FF>("Sumcheck:gate_challenge_" + std::to_string(idx));
    }

    // Parse out the aggregation object using the key->pairing_point_accumulator_public_input_indices
    // TODO(https://github.com/AztecProtocol/barretenberg/issues/1325): Eventually vk stores public input key directly.
    const PublicComponentKey pairing_point_public_input_key{ key->pairing_point_accumulator_public_input_indices[0],
                                                             true };
    AggregationObject nested_agg_obj =
        PublicAggState::reconstruct(verification_key->public_inputs, pairing_point_public_input_key);
    // TODO(https://github.com/AztecProtocol/barretenberg/issues/995): generate this challenge properly.
    typename Curve::ScalarField recursion_separator =
        Curve::ScalarField::from_witness_index(builder, builder->add_variable(42));
    agg_obj.aggregate(nested_agg_obj, recursion_separator);

    // Execute Sumcheck Verifier and extract multivariate opening point u = (u_0, ..., u_{d-1}) and purported
    // multivariate evaluations at u
    // TODO(https://github.com/AztecProtocol/barretenberg/issues/1283): Suspicious get_value().
    const size_t log_circuit_size = numeric::get_msb(static_cast<uint32_t>(key->circuit_size.get_value()));
    Sumcheck sumcheck(log_circuit_size, transcript);

    // Receive commitments to Libra masking polynomials
    std::array<Commitment, NUM_LIBRA_COMMITMENTS> libra_commitments = {};
    if constexpr (Flavor::HasZK) {
        libra_commitments[0] = transcript->template receive_from_prover<Commitment>("Libra:concatenation_commitment");
    }
    SumcheckOutput<Flavor> sumcheck_output =
        sumcheck.verify(verification_key->relation_parameters, verification_key->alphas, gate_challenges);

    // For MegaZKFlavor: the sumcheck output contains claimed evaluations of the Libra polynomials
    if constexpr (Flavor::HasZK) {
        libra_commitments[1] = transcript->template receive_from_prover<Commitment>("Libra:grand_sum_commitment");
        libra_commitments[2] = transcript->template receive_from_prover<Commitment>("Libra:quotient_commitment");
    }
    // Execute Shplemini to produce a batch opening claim subsequently verified by a univariate PCS
    bool consistency_checked = true;
    ClaimBatcher claim_batcher{
        .unshifted = ClaimBatch{ commitments.get_unshifted(), sumcheck_output.claimed_evaluations.get_unshifted() },
        .shifted = ClaimBatch{ commitments.get_to_be_shifted(), sumcheck_output.claimed_evaluations.get_shifted() }
    };
    const BatchOpeningClaim<Curve> opening_claim =
        Shplemini::compute_batch_opening_claim(log_circuit_size,
                                               claim_batcher,
                                               sumcheck_output.challenge,
                                               Commitment::one(builder),
                                               transcript,
                                               Flavor::REPEATED_COMMITMENTS,
                                               Flavor::HasZK,
                                               &consistency_checked,
                                               libra_commitments,
                                               sumcheck_output.claimed_libra_evaluation);

    auto pairing_points = PCS::reduce_verify_batch_opening_claim(opening_claim, transcript);

    pairing_points[0] = pairing_points[0].normalize();
    pairing_points[1] = pairing_points[1].normalize();
    // TODO(https://github.com/AztecProtocol/barretenberg/issues/995): generate recursion separator challenge properly.
    agg_obj.aggregate(pairing_points, recursion_separator);
    output.agg_obj = std::move(agg_obj);

    // Extract the IPA claim from the public inputs
    // Parse out the nested IPA claim using key->ipa_claim_public_input_indices and run the native IPA verifier.
    if constexpr (HasIPAAccumulator<Flavor>) {
        using PublicIpaClaim = PublicInputComponent<OpeningClaim<grumpkin<Builder>>>;

        if (verification_key->verification_key->contains_ipa_claim) {
            PublicComponentKey ipa_claim_key{ verification_key->verification_key->ipa_claim_public_input_indices[0],
                                              true };
            output.ipa_opening_claim = PublicIpaClaim::reconstruct(verification_key->public_inputs, ipa_claim_key);
        }
    }

    return output;
}

template class UltraRecursiveVerifier_<bb::UltraRecursiveFlavor_<UltraCircuitBuilder>>;
template class UltraRecursiveVerifier_<bb::UltraRecursiveFlavor_<MegaCircuitBuilder>>;
template class UltraRecursiveVerifier_<bb::MegaRecursiveFlavor_<UltraCircuitBuilder>>;
template class UltraRecursiveVerifier_<bb::MegaRecursiveFlavor_<MegaCircuitBuilder>>;
template class UltraRecursiveVerifier_<bb::MegaZKRecursiveFlavor_<MegaCircuitBuilder>>;
template class UltraRecursiveVerifier_<bb::MegaZKRecursiveFlavor_<UltraCircuitBuilder>>;
template class UltraRecursiveVerifier_<bb::UltraRecursiveFlavor_<CircuitSimulatorBN254>>;
template class UltraRecursiveVerifier_<bb::MegaRecursiveFlavor_<CircuitSimulatorBN254>>;
template class UltraRecursiveVerifier_<bb::UltraRollupRecursiveFlavor_<UltraCircuitBuilder>>;
} // namespace bb::stdlib::recursion::honk
