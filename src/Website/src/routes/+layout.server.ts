import { get_doc_names } from '$lib/helpers/DocFetcher';
import type { PageServerLoad } from './$types';
export const load: PageServerLoad = async () => {
    return {
        doc_names: await get_doc_names()
    }
};