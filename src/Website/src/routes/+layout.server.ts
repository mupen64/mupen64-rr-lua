import { get_doc_paths } from '$lib/helpers/DocFetcher';
import type { PageServerLoad } from './$types';
export const load: PageServerLoad = async () => {
    return {
        files: await get_doc_paths()
    }
};