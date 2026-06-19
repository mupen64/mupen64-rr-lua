import { get_doc_tree } from '$lib/helpers/DocFetcher';
import type { LayoutServerLoad } from './$types';

export const load: LayoutServerLoad = () => {
    return {
        doc_tree: get_doc_tree()
    }
};
